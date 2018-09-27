//
//  BREthereumLES.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 5/01/18.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "BRInt.h"
#include "BRArray.h"

#include "../rlp/BRRlp.h"
#include "../util/BRUtil.h"
#include "../base/BREthereumBase.h"

#include "BREthereumLES.h"
#include "BREthereumLESRandom.h"
#include "BREthereumMessage.h"
#include "BREthereumNode.h"

#if defined (__ANDROID__)
#include "../event/pthread_android.h"
#endif

/** Forward Declarations */

static void
lesHandleProvision (BREthereumLES les,
                    BREthereumNode node,
                    BREthereumProvisionResult result);

static void
lesHandleStatus (BREthereumLES les,
                 BREthereumNode node,
                 BREthereumHash headHash,
                 uint64_t headNumber);

static void
lesHandleAnnounce (BREthereumLES les,
                   BREthereumNode node,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   UInt256 headTotalDifficulty,
                   uint64_t reorgDepth);

static void
lesHandleNeighbor (BREthereumLES les,
                   BREthereumNode node,
                   BRArrayOf(BREthereumDISNeighbor) neighbors);

static ssize_t
lesFindRequestForProvision (BREthereumLES les,
                            BREthereumProvision *provision);

typedef void* (*ThreadRoutine) (void*);

static void *
lesThread (BREthereumLES les);

static inline int maximum (int a, int b) { return a > b ? a : b; }
#pragma clang diagnostic ignored "-Wunused-function"
static inline int minimum (int a, int b) { return a < b ? a : b; }
#pragma clang diagnostic pop

#define LES_THREAD_NAME    "Core Ethereum LES"
#define LES_PTHREAD_STACK_SIZE (512 * 1024)
#define LES_PTHREAD_NULL   ((pthread_t) NULL)

#define LES_REQUESTS_INITIAL_SIZE   10
#define LES_NODE_INITIAL_SIZE   10

#define LES_PREFERRED_NODE_INDEX     0

// For a request with a nodeIndex that is not LES_PREFERRED_NODE_INDEX, the intention is to send
// the same message multiple times.  If the message was to be sent to 3 nodes, but only two are
// connected, how many times should the message be sent and to what nodes?  If the following,
// LES_WRAP_SEND_WHEN_MULTIPLE_INDEX, is '1' when we'll send 3 times, including multiple times.  If
// the following is '0' then we'll multiple times up to the number of connected nodes.
#define LES_WRAP_SEND_WHEN_MULTIPLE_INDEX    1

// Submit a transaction multiple times.
#define LES_SUBMIT_TRANSACTION_COUNT 3

// Iterate over LES nodes...
#define FOR_SET(type,var,set) \
  for (type var = BRSetIterate(set, NULL); \
       NULL != var; \
       var = BRSetIterate(set, var))

#define FOR_NODES( les, node )  FOR_SET(BREthereumNode, node, les->nodes)

#define FOR_CONNECTED_NODES_INDEX( les, index ) \
    for (size_t index = 0; index < array_count ((les)->connectedNodes); index++)

#define FOR_AVAILABLE_NODES_INDEX( les, index ) \
    for (size_t index = 0; index < array_count ((les)->availableNodes); index++)

#define FOR_DISCOVERY_NODES_INDEX( les, index ) \
    for (size_t index = 0; index < array_count ((les)->discoveryNodes); index++)

/// MARK: - LES Node Config

struct BREthereumNodeConfigRecord {
    /** Hash of the public key */
    BREthereumHash hash;

    /** public key */
    BRKey key;

    /** DIS endpoint w/ IP addr and ports */
    BREthereumDISEndpoint endpoint;

    /** current state */
    BREthereumNodeState state;
};

extern void
nodeConfigRelease (BREthereumNodeConfig config) {
    free (config);
}

extern BREthereumHash
nodeConfigGetHash (BREthereumNodeConfig config) {
    return config->hash;
}

extern BRRlpItem
nodeConfigEncode (BREthereumNodeConfig config,
                     BRRlpCoder coder) {
    return rlpEncodeList (coder, 3,
                          rlpEncodeBytes(coder, config->key.pubKey, 65),
                          endpointDISEncode(&config->endpoint, coder),
                          nodeStateEncode(&config->state, coder));
}

extern BREthereumNodeConfig
nodeConfigDecode (BRRlpItem item,
                     BRRlpCoder coder) {
    BREthereumNodeConfig config = calloc (1, sizeof (struct BREthereumNodeConfigRecord));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);
    assert (3 == itemsCount);

    BRRlpData keyData = rlpDecodeBytesSharedDontRelease (coder, items[0]);
    BRKeySetPubKey(&config->key, keyData.bytes, keyData.bytesCount);

    config->endpoint = endpointDISDecode(items[1], coder);
    config->state = nodeStateDecode (items[2], coder);

    return config;
}

static BREthereumNodeConfig
nodeConfigCreate (BREthereumNode node) {
    BREthereumNodeConfig config = calloc (1, sizeof (struct BREthereumNodeConfigRecord));

    BREthereumNodeEndpoint *ne = nodeGetRemoteEndpoint(node);

    config->key = ne->dis.key;
    config->endpoint = ne->dis.node;
    config->state = nodeGetState(node, NODE_ROUTE_TCP);

    config->hash = hashCreateFromData((BRRlpData) { 64, &config->key.pubKey[1] });

    return config;
}

static BREthereumNodeEndpoint
nodeConfigCreateEndpoint (BREthereumNodeConfig config) {
    return nodeEndpointCreate ((BREthereumDISNeighbor) { config->endpoint, config->key });
}

/**
 * A LES Request is a LES Message with associated callbacks.  We'll send the message (once we have
 * connected to a LES node) and then wait for a response with the corresponding `requestId`.  Once
 * the response is fully constituted, we'll invoke the `callback` w/ `context` and w/ data from
 * both the request and response.
 *
 * Eventually the union subtype fields, like `BREthereumHash* transactions1` will disappear
 * because that data is held in `message`.
 *
 * Eventually the individual types for Callback+Context must disappear...
 */
typedef struct {
//    uint64_t requestId;

    BREthereumLESProvisionContext context;
    BREthereumLESProvisionCallback callback;
    BREthereumProvision provision;

    /**
     * The node that sent this request. This will be NULL until that request has been successfully
     * sent.  Once sent, if this request has not been received/resolved and the node is no longer
     * connected, then we'll resend.
     */
    BREthereumNode node;
//
//    /**
//     * The node index to use when sending.  Most will use the PREFFERED_NODE_INDEX of 0 - which is
//     * the default node.  But, for a SendTx message, we'll send to multiple nodes,
//     * if connected.
//     */
//    size_t nodeIndex;
} BREthereumLESRequest;

/// MARK: - LES

/**
 * LES
 */
struct BREthereumLESRecord {

    /** Some private key */
    BRKey key;

    /** Network */
    BREthereumNetwork network;

    /** RLP Coder */
    BRRlpCoder coder;

    /** Callbacks */
    BREthereumLESCallbackContext callbackContext;
    BREthereumLESCallbackAnnounce callbackAnnounce;
    BREthereumLESCallbackStatus callbackStatus;
    BREthereumLESCallbackSaveNodes callbackSaveNodes;

    /** Our Local Endpoint. */
    BREthereumNodeEndpoint localEndpoint;

    /** Nodes - all known */
    BRSetOf(BREthereumNode) nodes;

    /** Available Nodes - subset of `nodes`, ordered by 'DIS Distance'.  All have a TCP status of
     * 'AVAILABLE'; none are in `activeNodesByRoute[NODE_ROUTE_TCP]`.  Because these are ordered,
     * we can just select to first oneo for use */
    BRArrayOf(BREthereumNode) availableNodes;

    /** Active Nodes - a subset of `nodes` in a state of 'CONNECTED' or 'CONNECTING'.  We actively
     * `select()` on these nodes to hanlde send/recv needs */
    BRArrayOf(BREthereumNode) activeNodesByRoute[NUMBER_OF_NODE_ROUTES];

    /** Requests - pending, have not been provisioned to a node */
    BRArrayOf (BREthereumLESRequest) requests;

    /** Unique request identifier */
    BREthereumProvisionIdentifier requestsIdentifier;

    /** Thread */
    pthread_t thread;
    pthread_mutex_t lock;

    /** replace with pipe() message */
    int theTimeToQuitIsNow;
};

static void
assignLocalEndpointHelloMessage (BREthereumNodeEndpoint *endpoint) {
    // From https://github.com/ethereum/wiki/wiki/ÐΞVp2p-Wire-Protocol on 2019 Aug 21
    // o p2pVersion: Specifies the implemented version of the P2P protocol. Now must be 1
    // o listenPort: specifies the port that the client is listening on (on the interface that the
    //    present connection traverses). If 0 it indicates the client is not listening.
    BREthereumP2PMessage hello = {
        P2P_MESSAGE_HELLO,
        { .hello  = {
            P2P_MESSAGE_VERSION,
            strdup (LES_LOCAL_ENDPOINT_NAME),
            NULL, // capabilities
            endpoint->dis.node.portTCP,
            {}
        }}};

    //
    // Extend `capabilities` with supported subprotocols: LESv2 and/or PIPv1
    //
    array_new (hello.u.hello.capabilities, 2);

#if defined (LES_SUPPORT_GETH)
    array_add (hello.u.hello.capabilities, ((BREthereumP2PCapability) { "les", 2 }));
#endif

#if defined (LES_SUPPORT_PARITY)
    array_add (hello.u.hello.capabilities, ((BREthereumP2PCapability) { "pip", 1 }));
#endif

    // The NodeID is the 64-byte (uncompressed) public key
    uint8_t pubKey[65];
    assert (65 == BRKeyPubKey (&endpoint->dis.key, pubKey, 65));
    memcpy (hello.u.hello.nodeId.u8, &pubKey[1], 64);

    nodeEndpointSetHello (endpoint, hello);
    messageP2PHelloShow (hello.u.hello);
}

static BREthereumNode
lesEnsureNodeForEndpoint (BREthereumLES les,
                          BREthereumNodeEndpoint endpoint,
                          BREthereumNodeState state,
                          BREthereumBoolean preferred,
                          BREthereumBoolean *added) {
    BREthereumHash hash = endpoint.hash;

    pthread_mutex_lock (&les->lock);

    // Lookup an existing node.
    BREthereumNode node = BRSetGet (les->nodes, &hash);

    // If there is none, then note `added`...
    if (NULL != added) *added = AS_ETHEREUM_BOOLEAN (NULL == node);

    // ... and then actually add it.
    if (NULL == node) {
        // This endpoint is new so we'll create a node and then ...
        node = nodeCreate (les->network,
                           endpoint,
                           les->localEndpoint,
                           (BREthereumNodeContext) les,
                           (BREthereumNodeCallbackStatus) lesHandleStatus,
                           (BREthereumNodeCallbackAnnounce) lesHandleAnnounce,
                           (BREthereumNodeCallbackProvide) lesHandleProvision,
                           (BREthereumNodeCallbackNeighbor) lesHandleNeighbor);
        nodeSetStateInitial (node, NODE_ROUTE_TCP, state);

        // ... add it to 'all nodes'
        BRSetAdd(les->nodes, node);

        // .... and then, if warranted, make it available
        if (nodeHasState(node, NODE_ROUTE_TCP, NODE_AVAILABLE)) {
            if (ETHEREUM_BOOLEAN_IS_TRUE (preferred))
                array_insert (les->availableNodes, 0, node);
            else {
                BREthereumBoolean inserted = ETHEREUM_BOOLEAN_FALSE;
                for (size_t index = 0; index < array_count(les->availableNodes); index++)
                    // A simple, slow, linear-sort insert.
                    if (ETHEREUM_COMPARISON_LT == nodeNeighborCompare(node, les->availableNodes[index])) {
                        array_insert (les->availableNodes, index, node);
                        inserted = ETHEREUM_BOOLEAN_TRUE;
                        break;
                    }
                if (ETHEREUM_BOOLEAN_IS_FALSE(inserted)) array_add (les->availableNodes, node);
            }
        }
    }

    pthread_mutex_unlock (&les->lock);
    return node;
}

extern int
nodeEndpointIsPreferred (BREthereumNodeEndpoint *endpoint,
                         const char *enodes[]) {
    BREthereumDISNeighborEnode enode = neighborDISAsEnode (endpoint->dis, 1);
    for (size_t index = 0; NULL != enodes[index]; index++)
        if (0 == strcmp (enode.chars, enodes[index]))
            return 1;
    return 0;
}

//
// Public functions
//
extern BREthereumLES
lesCreate (BREthereumNetwork network,
           BREthereumLESCallbackContext callbackContext,
           BREthereumLESCallbackAnnounce callbackAnnounce,
           BREthereumLESCallbackStatus callbackStatus,
           BREthereumLESCallbackSaveNodes callbackSaveNodes,
           BREthereumHash headHash,
           uint64_t headNumber,
           UInt256 headTotalDifficulty,
           BREthereumHash genesisHash,
           BRArrayOf(BREthereumNodeConfig) configs) {
    
    BREthereumLES les = (BREthereumLES) calloc (1, sizeof(struct BREthereumLESRecord));
    assert (NULL != les);

    // For now, create a new, random private key that is used for communication with LES nodes.
    UInt256 secret;
#if defined (__ANDROID__)
    assert (0);
#else
    arc4random_buf(secret.u64, sizeof (secret));
#endif

    // Assign the generated private key.
    BRKeySetSecret(&les->key, &secret, 0);

    // Save the network.
    les->network = network;

    // Get our shared rlpCoder.
    les->coder = rlpCoderCreate();

    // Save callbacks.
    les->callbackContext = callbackContext;
    les->callbackAnnounce = callbackAnnounce;
    les->callbackStatus = callbackStatus;
    les->callbackSaveNodes = callbackSaveNodes;

    {
        // Use the privateKey to create a randomContext
        BREthereumLESRandomContext randomContext =  randomCreate (les->key.secret.u8, 32);

        // Create a local endpoint; when creating nodes we'll use this local endpoint repeatedly.
        les->localEndpoint = nodeEndpointCreateLocal(randomContext);

        randomRelease (randomContext);
    }

    // The 'hello' message is fixed; assign it to the local endpoint. We'll support local
    // capabilities as [ { "les", 2 }, { "pip", 1 } ] - but then require the remote to have one of
    // the two.  Depending on the shared capability, we'll assign the node type as GETH or PARITY.
    assignLocalEndpointHelloMessage (&les->localEndpoint);

    // The 'status' message is not fixed; create one and assign it to the local endpoint.  The
    // status message likely depends on the node type, GETH or PARITY. Specifically the
    // `protocolVersion` is 2 for GETH (our expectation) and 1 for PARITY ; `announceType` is only
    // specified for a GETH protocol of 2.
    //
    // We'll create a 'status' message now but modify it later once the local and remote endpoints
    // have exchanged hello messages.  We create this as a LES message but will reassign to
    // a PIP message if connected to a Parity node
    //
    // This is the only place where { headNumber, headHash, headTotalDifficult, genesitHash} are
    // preserved.
    BREthereumMessage status = {
        MESSAGE_LES,
        { .les = {
            LES_MESSAGE_STATUS,
            { .status = messageLESStatusCreate (0x02,  // LES v2
                                                networkGetChainId(network),
                                                headNumber,
                                                headHash,
                                                headTotalDifficulty,
                                                genesisHash,
                                                0x01) }}} // Announce type (of LES v2)
    };
    nodeEndpointSetStatus(&les->localEndpoint, status);

    // Create the PTHREAD LOCK variable
    {
        // The cacheLock is a normal, non-recursive lock
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); // overkill, but needed still
        pthread_mutex_init(&les->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    les->thread = LES_PTHREAD_NULL;

    // Initialize requests
    les->requestsIdentifier = 0;
    array_new (les->requests, LES_REQUESTS_INITIAL_SIZE);

    // The Set of all known nodes.
    les->nodes = BRSetNew (nodeHashValue,
                           nodeHashEqual,
                           10 * LES_NODE_INITIAL_SIZE);

    // (Sorted by Distance) array of available Nodes
    array_new (les->availableNodes, LES_NODE_INITIAL_SIZE);

    FOR_EACH_ROUTE (route)
        array_new (les->activeNodesByRoute[route], LES_NODE_INITIAL_SIZE);

    // Identify a set of initial nodes; first, use all the endpoints provided (based on `configs`)
    eth_log(LES_LOG_TOPIC, "Nodes Provided    : %lu", (NULL == configs ? 0 : array_count(configs)));
    if (NULL != configs) {
        BRArrayOf(BREthereumNodeEndpoint) preferredEndpoints;
        array_new (preferredEndpoints, 5);

        for (size_t index = 0; index < array_count(configs); index++) {
            BREthereumNodeEndpoint endpoint = nodeConfigCreateEndpoint(configs[index]);

            if (nodeEndpointIsPreferred (&endpoint, bootstrapLCLEnodes) ||
                nodeEndpointIsPreferred (&endpoint, bootstrapBRDEnodes))
                array_add (preferredEndpoints, endpoint);
            else
                lesEnsureNodeForEndpoint (les,
                                          endpoint,
                                          configs[index]->state,
                                          ETHEREUM_BOOLEAN_FALSE,
                                          NULL);
        }

        for (size_t index = 0; index < array_count(preferredEndpoints); index++)
            lesEnsureNodeForEndpoint (les,
                                      preferredEndpoints[index],
                                      (BREthereumNodeState) { NODE_AVAILABLE},
                                      ETHEREUM_BOOLEAN_TRUE,
                                      NULL);
    }

    // ... and then add in bootstrap endpoints for good measure.  Note that in practice after the
    // first boot, *none* of these nodes will be added as they would already be in 'configs' above.
    size_t bootstrappedEndpointsCount = 0;
    BREthereumBoolean bootstrappedEndpointsAdded;
    for (size_t set = 0; set < NUMBER_OF_NODE_ENDPOINT_SETS; set++) {
        const char **enodes = bootstrapMainnetEnodeSets[set];
        for (size_t index = 0; NULL != enodes[index]; index++) {
            lesEnsureNodeForEndpoint(les,
                                     nodeEndpointCreateEnode(enodes[index]),
                                     (BREthereumNodeState) { NODE_AVAILABLE },
                                     AS_ETHEREUM_BOOLEAN (enodes == bootstrapLCLEnodes ||
                                                          enodes == bootstrapBRDEnodes),
                                     &bootstrappedEndpointsAdded);
            if (ETHEREUM_BOOLEAN_IS_TRUE(bootstrappedEndpointsAdded))
                bootstrappedEndpointsCount++;
        }
    }
    eth_log(LES_LOG_TOPIC, "Nodes Bootstrapped: %lu", bootstrappedEndpointsCount);
    for (size_t index = 0; index < 5 && index < array_count(les->availableNodes); index++) {
        BREthereumDISNeighborEnode enode = neighborDISAsEnode (nodeGetRemoteEndpoint (les->availableNodes[index])->dis, 1);
        eth_log (LES_LOG_TOPIC, "  @ %zu: %s", index, enode.chars);
    }

    les->theTimeToQuitIsNow = 0;

    return les;
}

extern void
lesStart (BREthereumLES les) {
    pthread_mutex_lock (&les->lock);
    if (LES_PTHREAD_NULL == les->thread) {
        pthread_attr_t attr;
        pthread_attr_init (&attr);
        pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setstacksize (&attr, LES_PTHREAD_STACK_SIZE);

        les->theTimeToQuitIsNow = 0;
        pthread_create (&les->thread, &attr, (ThreadRoutine) lesThread, les);
        pthread_attr_destroy (&attr);
    }
    pthread_mutex_unlock (&les->lock);
}

extern void
lesStop (BREthereumLES les) {
    pthread_mutex_lock (&les->lock);
    if (LES_PTHREAD_NULL != les->thread) {
        les->theTimeToQuitIsNow = 1;
        // TODO: Unlock here - to avoid a deadlock on lock() after pselect()
        pthread_mutex_unlock (&les->lock);
        pthread_join (les->thread, NULL);
        les->thread = LES_PTHREAD_NULL;
    }
    pthread_mutex_unlock (&les->lock);
}

extern void
lesRelease(BREthereumLES les) {
    lesStop (les);
    pthread_mutex_lock (&les->lock);

    // Release each node; do it safely w/o mucking the les->nodes set.
    size_t nodeCount = BRSetCount(les->nodes);
    BREthereumNode nodes[nodeCount];
    BRSetAll(les->nodes, (void**) nodes, nodeCount);
    BRSetClear(les->nodes);
    for (size_t index = 0; index < nodeCount; index++)
        nodeRelease (nodes[index]);

//    FOR_NODES (les, node)
//        nodeRelease (node);
//    BRSetClear(les->nodes);

    array_free (les->availableNodes);

    FOR_EACH_ROUTE (route)
        array_free (les->activeNodesByRoute[route]);

    rlpCoderRelease(les->coder);

    // requests, requestsToSend

    // TODO: NodeEnpdoint Release (to release 'hello' and 'status' messages

    pthread_mutex_unlock (&les->lock);
    pthread_mutex_destroy (&les->lock);
    free (les);
}

static BREthereumNode
lesNodeFindDiscovery (BREthereumLES les) {
    // Look from among connected TCP nodes
    for (size_t index = 0; index < array_count (les->activeNodesByRoute[NODE_ROUTE_TCP]); index++) {
        BREthereumNode node = (les->activeNodesByRoute[NODE_ROUTE_TCP])[index];
        if (ETHEREUM_BOOLEAN_IS_FALSE(nodeGetDiscovered(node)) && nodeHasState(node, NODE_ROUTE_UDP, NODE_AVAILABLE))
            return node;
    }

    // Look from among available nodes
    for (size_t index = 0; index < array_count (les->availableNodes); index++) {
        BREthereumNode node = les->availableNodes[index];
        if (ETHEREUM_BOOLEAN_IS_FALSE(nodeGetDiscovered(node)) && nodeHasState(node, NODE_ROUTE_UDP, NODE_AVAILABLE))
            return node;
    }

    // Otherwise, look for any node
    FOR_NODES(les, node) {
        if (ETHEREUM_BOOLEAN_IS_FALSE(nodeGetDiscovered(node)) && nodeHasState(node, NODE_ROUTE_UDP, NODE_AVAILABLE))
            return node;
    }

    // And now.... we have nothing... mark all nodes as as UNDISCOVERED and AVAILABLE and try again.
    FOR_NODES(les, node) {
        nodeSetDiscovered (node, ETHEREUM_BOOLEAN_FALSE);
        nodeSetStateInitial (node, NODE_ROUTE_UDP, (BREthereumNodeState) { NODE_AVAILABLE });
    }

    // And if we have no nodes...  add the bookstrap ones back, again - how did they disappear?
    assert (BRSetCount(les->nodes) > 0);

    return lesNodeFindDiscovery (les);
}

///
/// MARK: - LES Node Callbacks
///

/**
 * Handle a Node's Status message by invoking les->callbackStatus with the Node's head{Hash,Num}
 *
 * @param les LES
 * @param node The node
 * @param headHash The Node's Head hash.
 * @param headNumber The Node's Head number
 *
 * @note This is always called from the Node's 'Connect Thread'
 */
static void
lesHandleStatus (BREthereumLES les,
                 BREthereumNode node,
                 BREthereumHash headHash,
                 uint64_t headNumber) {
    les->callbackStatus (les->callbackContext,
                         headHash,
                         headNumber);
}

/**
 * Handle a Node's Account message by invoking the les->callbackAnnounce with the announced
 * block's properties - head{Hash,Number,TotalDifficulty} - along with the Node's reordDepth
 *
 * @param les LES
 * @param node The node
 * @param headHash The block's head hash
 * @param headNumber The block's head number
 * @param headTotalDifficulty The block's total difficulty
 * @param reorgDepth The node's reorganization depth
 *
 * @note This is always called from the LES 'Main Thread'
 */
static void
lesHandleAnnounce (BREthereumLES les,
                   BREthereumNode node,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   UInt256 headTotalDifficulty,
                   uint64_t reorgDepth) {
    les->callbackAnnounce (les->callbackContext,
                           headHash,
                           headNumber,
                           headTotalDifficulty,
                           reorgDepth);
}


/**
 * Handle a Node's Provision result by invoking the result's callback.  On success, the result
 * is everything requested from LES - such as Block Header, Block Bodies, ..., Account States.
 *
 * @param les LES
 * @param node The node
 * @param result The node's provision reslt.
 *
 * @note This is always called from the LES 'Main Thread'
 */
static void
lesHandleProvision (BREthereumLES les,
                    BREthereumNode node,
                    BREthereumProvisionResult result) {
    // Find the request, invoke the callbacks on result, and others.
    // TODO: On an error, should the provision be submitted to another node?
    for (size_t index = 0; index < array_count (les->requests); index++) {
        BREthereumLESRequest *request = &les->requests[index];
        if (result.identifier == request->provision.identifier) {
            request->callback (request->context,
                               les,
                               node,
                               result);
            array_rm (les->requests, index);
            break;
        }
    }
}

/**
 * Check if there is already a node for neighbor.  If there is, do nothing; if there isn't then
 * create a node and add it.
 */


/**
 * Handle a Node's Neighbors result by adding the neighbor, if unknown, as a new node.
 *
 * @param les LES
 * @param node The node
 * @param neighbor The neighbor
 * @param remaining The number of neighbors remaining to be handled.
 *
 * @return If added, TRUE; otherwise FALSE
 *
 * @note: This is always called from the XYZ 'Foo Thread'
 */
static void
lesHandleNeighbor (BREthereumLES les,
                   BREthereumNode node,
                   BRArrayOf(BREthereumDISNeighbor) neighbors) {
    for (size_t index = 0; index < array_count(neighbors); index++)
        lesEnsureNodeForEndpoint (les,
                                  nodeEndpointCreate(neighbors[index]),
                                  (BREthereumNodeState) { NODE_AVAILABLE },
                                  ETHEREUM_BOOLEAN_FALSE,
                                  NULL);
    // Sometimes we receive multiple callbacks for one FIND_NEIGHBORS - how to deal?
    nodeSetDiscovered(node, ETHEREUM_BOOLEAN_TRUE);
}

///
/// MARK: - LES (Main) Thread
///

static void
lesLogNodeActivate (BREthereumLES les,
                    BREthereumNode node,
                    BREthereumNodeEndpointRoute route,
                    const char *explain,
                    const char *path) {
    char desc[128];

    BREthereumNodeState state = nodeGetState(node, route);
    eth_log (LES_LOG_TOPIC, "Conn: [ %s @ %3zu, %9s ]    %15s (%s)",
             (NODE_ROUTE_TCP == route ? "TCP" : "UDP"),
             array_count(les->activeNodesByRoute[route]),
             path,
             nodeGetRemoteEndpoint(node)->hostname,
//             explain,
             nodeStateDescribe (&state, desc));
}

static void
lesDeactivateNodes (BREthereumLES les,
                    BREthereumNodeEndpointRoute route,
                    BRArrayOf(BREthereumNode) nodesToDeactivate,
                    const char *explain) {
    // Deactive nodesToDeactive from the route's activeNodes.
    BRArrayOf(BREthereumNode) nodes = les->activeNodesByRoute[route];
    for (size_t ri = 0; ri < array_count(nodesToDeactivate); ri++) {  // RemoveIndex
        for (size_t ni = 0; ni < array_count(nodes); ni++) // NodeIndex
            if (nodesToDeactivate[ri] == nodes[ni]) {
                array_rm (nodes, ni);
                lesLogNodeActivate(les, nodesToDeactivate[ri], route, explain, "<=|=>");

                // Reassign provisions back as requests if this is a TCP route
                if (NODE_ROUTE_TCP == route) {
                    BRArrayOf(BREthereumProvision) provisions = nodeUnhandleProvisions(nodesToDeactivate[ri]);
                    for (size_t pi = 0; pi < array_count(provisions); pi++) {
                        ssize_t requestIndex = lesFindRequestForProvision (les, &provisions[pi]);
                        assert (-1 != requestIndex);

                        // This reestablishes the provision as needing to be assigned.
                        les->requests[requestIndex].node = NULL;
                    }
                }

                // A node was deactiviated (and `nodes` modified); break to deactivate another.
                break;
            }
    }
}

static void
lesHandleSelectError (BREthereumLES les,
                      int error) {
    switch (error) {
        case EAGAIN:
        case EBADF:
        case EINTR:
        case EINVAL:
            eth_log (LES_LOG_TOPIC, "Select Error: %s", strerror(error));
            break;
    }
}

static void *
lesThread (BREthereumLES les) {
#if defined (__ANDROID__)
    pthread_setname_np (les->thread, LES_THREAD_NAME);
#else
    pthread_setname_np (LES_THREAD_NAME);
#endif
    pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);
    // pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);

    // TODO: Don't timeout pselect(); get some 'wakeup descriptor'
    struct timespec timeout = { 0, 250000000 }; // .250 seconds

    //
    fd_set readDescriptors, writeDesciptors;
    int maximumDescriptor = -1;

    // True if we need to update descriptors.
    int updateDesciptors = 1;

    pthread_mutex_lock (&les->lock);

    BRArrayOf(BREthereumNode) nodesToRemove;
    array_new(nodesToRemove, 10);

    while (!les->theTimeToQuitIsNow) {
        time_t now = time (NULL);

        //
        // Check all available nodes for a timeout.  We check on every loop.  Instead, we could
        // check only when the subsequent `pselect()` times out; however, imagine we have one
        // node that we are actively communicating with.  In such a case the `pselect()` might
        // never timeout but all other nodes could be dead.  Catch the dead nodes up front.
        //
        FOR_EACH_ROUTE (route) {
            BRArrayOf(BREthereumNode) nodes = les->activeNodesByRoute[route];
            for (size_t index = 0; index < array_count(nodes); index++)
                if (ETHEREUM_BOOLEAN_IS_TRUE (nodeHandleTime (nodes[index], route, now))) {
                    array_add (nodesToRemove, nodes[index]);
                    // TODO: Reassign provisions
                }
            lesDeactivateNodes(les, route, nodesToRemove, "TIMEDOUT");
            array_clear (nodesToRemove);
        }

        //
        // Our preferredNode is an active TCP (P2P, ETH, LES, PIP) node at index 0;
        //
        BREthereumNode preferredNode = (array_count(les->activeNodesByRoute[NODE_ROUTE_TCP]) > 0
                                        ? les->activeNodesByRoute[NODE_ROUTE_TCP][0]
                                        : NULL);

        //
        // Handle any/all pending requests by 'establishing a provision' in the preferred node.
        //
        if (NULL != preferredNode && nodeHasState(preferredNode, NODE_ROUTE_TCP, NODE_CONNECTED))
            for (size_t index = 0; index < array_count (les->requests); index++)
                // Only handle a reqeust if it hasn't been previously handled.
                if (NULL == les->requests[index].node) {
                    // What about a 'multiple request', like a TxRelay, that is supposed to
                    // be sent/provisioned by multiple nodes?
                    les->requests[index].node = preferredNode;
                    nodeHandleProvision (preferredNode,
                                         les->requests[index].provision);
                }

        // Just do it, always.
        updateDesciptors = 1;

        //
        // Update the read (and write) descriptors to include nodes that are 'active' on any route.
        //
        if (updateDesciptors) {
            maximumDescriptor = -1;
            FD_ZERO (&readDescriptors);
            FD_ZERO (&writeDesciptors);

            FOR_EACH_ROUTE(route) {
                BRArrayOf(BREthereumNode) nodes = les->activeNodesByRoute[route];
                for (size_t index = 0; index < array_count(nodes); index++)
                    maximumDescriptor = maximum (maximumDescriptor,
                                                 nodeUpdateDescriptors (nodes[index],
                                                                        route,
                                                                        &readDescriptors,
                                                                        &writeDesciptors));
            }

            // updateDesciptors = 0;
        }

        pthread_mutex_unlock (&les->lock);
        int selectCount = pselect (1 + maximumDescriptor, &readDescriptors, &writeDesciptors, NULL, &timeout, NULL);
        pthread_mutex_lock (&les->lock);
        if (les->theTimeToQuitIsNow) continue;

        //
        // We have one or more nodes ready to process ...
        //
        if (selectCount > 0) {
            FOR_EACH_ROUTE (route) {
                BRArrayOf(BREthereumNode) nodes = les->activeNodesByRoute[route];
                for (size_t index = 0; index < array_count(nodes); index++) {
                    BREthereumNode node = nodes[index];

                    int isConnected = nodeHasState (node, route, NODE_CONNECTED);

                    // Process the node - based on the read/write descriptors.
                    nodeProcess (node, route, now, &readDescriptors, &writeDesciptors);

                    // Any node that is not CONNECTING or CONNECTED is no longer active.  Note that
                    // we can't just remove `node` at `index` because we are iterating on the array.
                    switch (nodeGetState(node, route).type) {
                        case NODE_AVAILABLE:
                        case NODE_ERROR:
                            array_add (nodesToRemove, node);
                            break;

                        case NODE_CONNECTING:
                            break;

                        case NODE_CONNECTED:
                            if (!isConnected && NODE_ROUTE_TCP == route)
                                lesLogNodeActivate(les, node, route, "", "<===>");
                            break;
                    }
                }

                lesDeactivateNodes (les, route, nodesToRemove, "SELECT");
                array_clear(nodesToRemove);
            }
        }

        //
        // or we have a timeout ... nothing to receive; nothing to send
        //
        else if (selectCount == 0) {

            // If a pending request was sent to a node that is no longer connected, then there
            // is no response coming (we are here because 'nothing to receive').  Resend.
            //
            // We are going to iterate back-to-front across les->requests and add each 'orphaned'
            // request to the front of les->requestsToSend - thereby keeping the order quasi-
            // consistent (not that it matters, until proven otherwise).

            // TODO: Retry
//            for (size_t index = array_count(les->requests); index > 0; index--) {
//                BREthereumNode node = les->requests[index - 1].node;
//                assert (NULL != node);
//                if (!nodeHasState (node, NODE_ROUTE_TCP, NODE_CONNECTED)) {
//                    // `request` is a value type - don't forget....
//                    BREthereumLESRequest request = les->requests[index - 1];
//                    array_rm (les->requests, (index - 1)); // safe, when going back-to-front
//                    request.node = NULL;
//                    array_insert (les->requestsToSend, 0, request); // to the front
//                    eth_log (LES_LOG_TOPIC, "Rtry: [ LES, %15s ]",
//                             messageLESGetIdentifierName (request.message.identifier));
//                }
//            }

            // If we don't have enough availableNodes, try to find some
            if (array_count(les->availableNodes) < 100 && array_count(les->activeNodesByRoute[NODE_ROUTE_UDP]) < 3) {

                // Find a 'discovery' node by looking in: activeNodesByRoute[NODE_ROUTE_TCP],
                // availableNodes and then finally allNodes.  If that fails, try harder (see
                // details in lesNodeFindDiscovery()
                BREthereumNode node = lesNodeFindDiscovery(les);

                // Try to connect...
                nodeConnect (node, NODE_ROUTE_UDP);

                // On success, make active
                switch (nodeGetState(node, NODE_ROUTE_UDP).type) {
                    case NODE_AVAILABLE:
                    case NODE_ERROR:
                        break;

                    case NODE_CONNECTING:
                        array_add(les->activeNodesByRoute[NODE_ROUTE_UDP], node);
                        lesLogNodeActivate(les, node, NODE_ROUTE_UDP, "", "<...>");
                        break;

                    case NODE_CONNECTED:
                        assert (0);  // how?
                }
            }

            // If we don't have enough connectedNodes, try to add one
            if (array_count(les->activeNodesByRoute[NODE_ROUTE_TCP]) < 5 &&
                array_count(les->availableNodes) > 0) {
                BREthereumNode node = les->availableNodes[0];

                nodeConnect (node, NODE_ROUTE_TCP);

                switch (nodeGetState(node, NODE_ROUTE_TCP).type) {
                    case NODE_AVAILABLE:
                        break;

                    case NODE_ERROR:
                        // On error; no longer available
                        lesLogNodeActivate(les, node, NODE_ROUTE_TCP, "", "<=|=>");
                        array_rm (les->availableNodes, 0);
                        break;

                    case NODE_CONNECTING:
                        array_rm (les->availableNodes, 0);
                        array_add(les->activeNodesByRoute[NODE_ROUTE_TCP], node);
                        lesLogNodeActivate(les, node, NODE_ROUTE_TCP, "", "<...>");
                        break;

                    case NODE_CONNECTED:
                        assert (0);  // how?
                }
            }

            // updateDesciptors = 1;

            // pipe ()
        }

        //
        // or we have an pselect() error.
        //
        else {
            lesHandleSelectError (les, errno);
            updateDesciptors = 1;
        }

        // double check that everything has been handled.
        assert (0 == array_count(nodesToRemove));

    } // end while (!les->theTimeToQuitIsNow)

    pthread_mutex_unlock (&les->lock);

    eth_log (LES_LOG_TOPIC, "Stop: Nodes: %zu, Available: %zu, Connected: [%zu, %zu]",
             BRSetCount(les->nodes),
             array_count (les->availableNodes),
             array_count (les->activeNodesByRoute[NODE_ROUTE_UDP]),
             array_count (les->activeNodesByRoute[NODE_ROUTE_TCP]));

    // Callback on Node Config
    BRArrayOf(BREthereumNodeConfig) configs;
    array_new (configs, BRSetCount(les->nodes));
    FOR_NODES (les, node) {
        array_add (configs, nodeConfigCreate(node));
    }
    les->callbackSaveNodes (les->callbackContext, configs);

    FOR_NODES (les, node) {
        nodeShow (node);
        nodeDisconnect (node, NODE_ROUTE_UDP, P2P_MESSAGE_DISCONNECT_REQUESTED);
        nodeDisconnect (node, NODE_ROUTE_TCP, P2P_MESSAGE_DISCONNECT_REQUESTED);
    }

    pthread_exit (0);
}

///
/// MARK: - (Public) Provide (Headers, ...)
///

static void
lesAddRequest (BREthereumLES les,
               BREthereumLESProvisionContext context,
               BREthereumLESProvisionCallback callback,
               BREthereumProvision provision) {
    pthread_mutex_lock (&les->lock);

    provision.identifier = les->requestsIdentifier++;

    BREthereumLESRequest request = { context, callback, provision, NULL };
    array_add (les->requests, request);
    pthread_mutex_unlock (&les->lock);
}

static ssize_t
lesFindRequestForProvision (BREthereumLES les,
                            BREthereumProvision *provision) {
    for (ssize_t index = 0; index < array_count (les->requests); index++)
        if (ETHEREUM_BOOLEAN_IS_TRUE (provisionMatches(provision, &les->requests[index].provision)))
            return index;
    return -1;
}

static BRArrayOf(BREthereumHash)
lesCreateHashArray (BREthereumLES les,
                    BREthereumHash hash) {
    BRArrayOf(BREthereumHash) hashes;
    array_new (hashes, 1);
    array_add (hashes, hash);
    return hashes;
}

extern void
lesProvideBlockHeaders (BREthereumLES les,
                        BREthereumLESProvisionContext context,
                        BREthereumLESProvisionCallback callback,
                        uint64_t start,  // Block Number
                        uint32_t limit,
                        uint64_t skip,
                        BREthereumBoolean reverse) {
    lesAddRequest (les, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_BLOCK_HEADERS,
                       { .headers = { start, skip, limit, reverse, NULL }}
                   });
}

extern void
lesProvideBlockBodies (BREthereumLES les,
                       BREthereumLESProvisionContext context,
                       BREthereumLESProvisionCallback callback,
                       BRArrayOf(BREthereumHash) blockHashes) {
    lesAddRequest (les, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_BLOCK_BODIES,
                       { .bodies = { blockHashes, NULL }}
                   });
}

extern void
lesProvideBlockBodiesOne (BREthereumLES les,
                          BREthereumLESProvisionContext context,
                          BREthereumLESProvisionCallback callback,
                          BREthereumHash blockHash) {
    lesProvideBlockBodies (les, context, callback,
                           lesCreateHashArray (les, blockHash));
}

extern void
lesProvideReceipts (BREthereumLES les,
                    BREthereumLESProvisionContext context,
                    BREthereumLESProvisionCallback callback,
                    BRArrayOf(BREthereumHash) blockHashes) {
    lesAddRequest (les, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_TRANSACTION_RECEIPTS,
                       { .receipts = { blockHashes, NULL }}
                   });
}

extern void
lesProvideReceiptsOne (BREthereumLES les,
                       BREthereumLESProvisionContext context,
                       BREthereumLESProvisionCallback callback,
                       BREthereumHash blockHash) {
    lesProvideReceipts (les, context, callback,
                        lesCreateHashArray(les, blockHash));
}

extern void
lesProvideAccountStates (BREthereumLES les,
                         BREthereumLESProvisionContext context,
                         BREthereumLESProvisionCallback callback,
                         BREthereumAddress address,
                         BRArrayOf(BREthereumHash) blockHashes,
                         BRArrayOf(uint64_t) blockNumbers) {
    lesAddRequest (les, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_ACCOUNTS,
                       { .accounts = { address, blockHashes, blockNumbers, NULL }}
                   });
}

extern void
lesProvideAccountStatesOne (BREthereumLES les,
                            BREthereumLESProvisionContext context,
                            BREthereumLESProvisionCallback callback,
                            BREthereumAddress address,
                            BREthereumHash blockHash,
                            uint64_t blockNumber) {
    BRArrayOf(uint64_t) blockNumbers;
    array_new (blockNumbers, 1);
    array_add (blockNumbers, blockNumber);
    lesProvideAccountStates (les, context, callback, address,
                             lesCreateHashArray(les, blockHash),
                             blockNumbers);
}

extern void
lesProvideTransactionStatus (BREthereumLES les,
                             BREthereumLESProvisionContext context,
                             BREthereumLESProvisionCallback callback,
                             BRArrayOf(BREthereumHash) transactionHashes) {
    lesAddRequest (les, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_TRANSACTION_STATUSES,
                       { .statuses = { transactionHashes, NULL }}
                   });
}

extern void
lesProvideTransactionStatusOne (BREthereumLES les,
                                BREthereumLESProvisionContext context,
                                BREthereumLESProvisionCallback callback,
                                BREthereumHash transactionHash) {
    lesProvideTransactionStatus (les, context, callback,
                                 lesCreateHashArray(les, transactionHash));
}

extern void
lesSubmitTransaction (BREthereumLES les,
                      BREthereumLESProvisionContext context,
                      BREthereumLESProvisionCallback callback,
                      BREthereumTransaction transaction) {
    lesAddRequest (les, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_SUBMIT_TRANSACTION,
                       { .submission = { transaction, {} }}
                   });
}

//static void
//lesSendAllProvisions (BREthereumLES les) {
//    pthread_mutex_lock (&les->lock);
//    // Handle all requestsToSend, if we can.
//    while (array_count (les->provisioners) > 0) {
//        size_t connectedNodesCount = array_count (les->connectedNodes);
//
//        // If we've no connected nodes, then there is no point continuing.  (I don't think this
//        // value can change while we've got the les->lock.  Anyways, 'belt-and-suspenders')
//        if (0 == connectedNodesCount) break;
//
//        // Get the next LES request.
//        BREthereumNodeProvisioner provisioner = les->provisioners[0];
//
//        // Get the request's desired nodeIndex.
//        size_t index = 0; // request.nodeIndex;
//
//        // If we are willing to wrap the index (by number of connected nodes), then do so.
//        // Example: connectedNodesCount = 2, index = 3 => index = 1.  Note: index = 3 implies
//        // we have four requests {0, 1, 2, 3 }; with connectedNodesCount = 2 we'll send to
//        // {0, 1, 0, 1 }.
//#if LES_WRAP_SEND_WHEN_MULTIPLE_INDEX
//        index %= connectedNodesCount;
//#endif
//
//        // If the desired index (via nodeIndex) is too large, then drop the request and continue.
//        // There is danger here - but note that we should have gotten at least on send off.
//        if (index >= connectedNodesCount) {
//            array_rm (les->provisioners, 0);
//            continue; // don't break; not an error.
//        }
//
//        // Cache the node - again 'belt-and-suspenders'
//        BREthereumNode node = les->connectedNodes[index];
//
//        nodeEstablishProvisioner(node, &provisioner);
//        
//        // And send the request's LES message
//        BREthereumNodeStatus status = nodeSend (node,
//                                                   NODE_ROUTE_TCP,
//                                                   (BREthereumMessage) {
//                                                       MESSAGE_LES,
//                                                       { .les = request.message }});
//
//        // If the send failed, then skip out to fight another day.
//        if (NODE_STATUS_ERROR == status) break;
//
//        // Mark as 'sent' (request is a value type - don't forget (for upcoming array_add)).
////        request.node = node;
//
//        // Transfer from `reqeustsToSend` over to `requests(Pending)`.
//        array_rm (les->provisioners, 0);
//
////        array_add (les->requests, request);
//    }
//    pthread_mutex_unlock (&les->lock);
//}

/**
 * lesSendAllRequests - Send all `requestsToSend` messages; when sent, move the request to
 * 'pending'.
 *
 * Each Node has a defined amount of 'credits'.  When a message is sent, the remote Node adjusts
 * the remaining credits (sometimes replenishing them as time goes by) and then reports the credits
 * in the response message.  We'd like to only send a request to a node w/ enough credits to handle
 * the reqeust successfully.
 *
 * Since we don't keep an estimate of our credits, the node's current credits could be way off when
 * this function, lesSendAllRequests(), is called.  Imagine we have N requests that have already
 * been sent but for which we've not processed a response yet (might not have arrived; might have
 * arrived but it is still queued (in the socket's buffer)).  We'll have X1 credits here but the
 * actual number will be X2 (< X1).
 *
 * We can't really tell if we have enough to send a request.  Options are:
 *
 * a) Only send a message if all responses have been received.  We'll then have a credit that is
 * consistent locally and remotely.  It surely feels impractical to send message one-by-one
 * synchronously.
 *
 * b) Send the message, credits be damned.  We'll move the messages to the 'pending' list and wait.
 * The remote node will disconnect us (USELESS_PEER, BREACH_PROTO?).  Okay, we can be disconnected
 * for lots of reasons and we need to handle all the others too.
 *
 * c) Track an estimate of the remaining credits - something like 'pendingCredits'.  To send a
 * message is must be that 'actualCredits - pendingCredit' > 0.  When message is sent, we increment
 * 'pendingCredits' and check - if good we send, if not we wait.  When a message arrives, we update
 * 'actualCredits' and reduce 'pendingCredits'.  (Since a request keeps the message around, we can
 * recompute the message credits by which to redcue 'pendingCredits').
 *
 * Conclusion: It seems that since 'b' must be handled anyways... we'll send off all requests and
 * if we get disconnected - due to credits - we'll handle that with all the other disconnect cases.
 *
 */

//static void
//lesSendAllRequests (BREthereumLES les) {
//    pthread_mutex_lock (&les->lock);
//    // Handle all requestsToSend, if we can.
//    while (array_count (les->requestsToSend) > 0) {
//        size_t connectedNodesCount = array_count (les->connectedNodes);
//
//        // If we've no connected nodes, then there is no point continuing.  (I don't think this
//        // value can change while we've got the les->lock.  Anyways, 'belt-and-suspenders')
//        if (0 == connectedNodesCount) break;
//
//        // Get the next LES request.
//        BREthereumLESReqeust request = les->requestsToSend[0];
//
//        // Get the request's desired nodeIndex.
//        size_t index = request.nodeIndex;
//
//        // If we are willing to wrap the index (by number of connected nodes), then do so.
//        // Example: connectedNodesCount = 2, index = 3 => index = 1.  Note: index = 3 implies
//        // we have four requests {0, 1, 2, 3 }; with connectedNodesCount = 2 we'll send to
//        // {0, 1, 0, 1 }.
//#if LES_WRAP_SEND_WHEN_MULTIPLE_INDEX
//        index %= connectedNodesCount;
//#endif
//
//        // If the desired index (via nodeIndex) is too large, then drop the request and continue.
//        // There is danger here - but note that we should have gotten at least on send off.
//        if (index >= connectedNodesCount) {
//            array_rm (les->requestsToSend, 0);
//            continue; // don't break; not an error.
//        }
//
//        // Cache the node - again 'belt-and-suspenders'
//        BREthereumNode node = les->connectedNodes[index];
//
//        // And send the request's LES message
//        BREthereumNodeStatus status = nodeSend (node,
//                                                   NODE_ROUTE_TCP,
//                                                   (BREthereumMessage) {
//                                                       MESSAGE_LES,
//                                                       { .les = request.message }});
//
//        // If the send failed, then skip out to fight another day.
//        if (NODE_STATUS_ERROR == status) break;
//
//        // Mark as 'sent' (request is a value type - don't forget (for upcoming array_add)).
//        request.node = node;
//
//        // Transfer from `reqeustsToSend` over to `requests(Pending)`.
//        array_rm (les->requestsToSend, 0);
//        array_add (les->requests, request);
//    }
//    pthread_mutex_unlock (&les->lock);
//}
