//
//  BREthereumLES.h
//  Core Ethereum
//
//  Created by Lamont Samuels on 5/01/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <netdb.h>
#include "support/BRInt.h"
#include "support/BRArray.h"
#include "ethereum/rlp/BRRlp.h"
#include "ethereum/util/BRUtil.h"
#include "ethereum/base/BREthereumBase.h"
#include "BREthereumLES.h"
#include "BREthereumLESRandom.h"
#include "BREthereumMessage.h"
#include "BREthereumNode.h"

#if !defined(LES_BOOTSTRAP_LCL_ONLY)
#   if defined (LES_BOOTSTRAP_BRD_ONLY)
static int bootstrapBRDOnly = 1;
#   else
static int bootstrapBRDOnly = 0;
#   endif
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

static void
lesDeactivateNode (BREthereumLES les,
                   BREthereumNodeEndpointRoute route,
                   BREthereumNode node,
                   const char *explain);

typedef void* (*ThreadRoutine) (void*);

static void *
lesThread (BREthereumLES les);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
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

    /** the priority */
    BREthereumNodePriority priority;
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
    return rlpEncodeList (coder, 4,
                          rlpEncodeBytes(coder, config->key.pubKey, 65),
                          endpointDISEncode(&config->endpoint, coder),
                          nodeStateEncode(&config->state, coder),
                          rlpEncodeUInt64(coder, config->priority, 0));
}

extern BREthereumNodeConfig
nodeConfigDecode (BRRlpItem item,
                     BRRlpCoder coder) {
    BREthereumNodeConfig config = calloc (1, sizeof (struct BREthereumNodeConfigRecord));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);
    assert (4 == itemsCount);

    BRRlpData keyData = rlpDecodeBytesSharedDontRelease (coder, items[0]);
    BRKeySetPubKey(&config->key, keyData.bytes, keyData.bytesCount);

    config->endpoint = endpointDISDecode(items[1], coder);
    config->state    = nodeStateDecode (items[2], coder);
    config->priority = (BREthereumNodePriority) rlpDecodeUInt64(coder, items[3], 0);

    config->hash = hashCreateFromData((BRRlpData) { 64, &config->key.pubKey[1] });

    return config;
}

static BREthereumNodeConfig
nodeConfigCreate (BREthereumNode node) {
    BREthereumNodeConfig config = calloc (1, sizeof (struct BREthereumNodeConfigRecord));

    BREthereumNodeEndpoint ne = nodeGetRemoteEndpoint(node);

    config->key = nodeEndpointGetDISNeighbor(ne).key;
    config->endpoint = nodeEndpointGetDISNeighbor(ne).node;
    config->state = nodeGetState(node, NODE_ROUTE_TCP);
    config->priority = nodeGetPriority (node);

    config->hash = hashCreateFromData((BRRlpData) { 64, &config->key.pubKey[1] });

    return config;
}

#if !defined(LES_BOOTSTRAP_LCL_ONLY)
static BREthereumNodeEndpoint
nodeConfigCreateEndpoint (BREthereumNodeConfig config) {
    return nodeEndpointCreate ((BREthereumDISNeighbor) { config->endpoint, config->key });
}
#endif

extern size_t
nodeConfigHashValue (const void *t)
{
    return hashSetValue(&((BREthereumNodeConfig) t)->hash);
}

extern int
nodeConfigHashEqual (const void *t1, const void *t2) {
    return t1 == t2 || hashSetEqual (&((BREthereumNodeConfig) t1)->hash,
                                     &((BREthereumNodeConfig) t2)->hash);
}

#if defined (INCLUDE_UNUSED_FUNCTION)
static inline void
nodeConfigReleaseForSet (void *ignore, void *item) {
    nodeConfigRelease ((BREthereumNodeConfig) item);
}
#endif

extern BREthereumNodeState
nodeGetPreferredState (BREthereumNodeState state) {
    // If `state` is an error state with an error of NODE_PROTOCOL_RLP_PARSE or
    // NODE_PROTOCOL_CAPABILITIES_MISMATCH, then DO NOT return a preferred state of
    // NODE_AVAILABLE.  A 'RLP_PARSE' error is serious; a 'CAPABILITIES_MISMATCH' will *never* be
    // corrected (that node id will change - I think - if the capabilities change - hmm, maybe not)
    return ((NODE_ERROR == state.type &&
             NODE_ERROR_PROTOCOL == state.u.error.type &&
             (NODE_PROTOCOL_RLP_PARSE == state.u.error.u.protocol ||
              NODE_PROTOCOL_CAPABILITIES_MISMATCH == state.u.error.u.protocol))
            ? state
            : ((BREthereumNodeState) { NODE_AVAILABLE }));
}


/**
 * A LES Request is a LES Message with associated callbacks.  We'll send the message (once we have
 * connected to a LES node) and then wait for a response with the corresponding `requestId`.  Once
 * the response is fully constituted, we'll invoke the `callback` w/ `context` and w/ data from
 * both the request and response.
 */
typedef struct {

    BREthereumLESProvisionContext context;
    BREthereumLESProvisionCallback callback;
    BREthereumProvision provision;

    /**
     * The node that should handle this reqeust or a GENERIC node (NIL, ANY, ALL).  If non-generic
     * we'll *must* use it when handling the provion - if we use any other node, it might not have
     * a suitable block-chain state.  If generic, we'll select some active node.
     */
    BREthereumNodeReference nodeReference;

    /**
     * The node that sent this request. This will be NULL until that request has been successfully
     * sent.  Once sent, if this request has not been received/resolved and the node is no longer
     * connected, then we'll resend.
     *
     * TODO: If resending, better continue to respect nodeReference.
     */
    BREthereumNode node;

} BREthereumLESRequest;

static void
requestRelease (BREthereumLESRequest *request) {
    // TODO: Figure out 'request/node provisions sharing memory'.

    // Don't release a provision if it is 'owned' by a `node` - the node will release it.
    if (NULL == request->node)
        provisionRelease(&request->provision, ETHEREUM_BOOLEAN_TRUE);
}

static void
requestsRelease (OwnershipGiven BRArrayOf(BREthereumLESRequest) requests) {
    if (NULL != requests) {
        for (size_t index = 0; index < array_count(requests); index++)
            requestRelease(&requests[index]);
        array_free(requests);
    }
}

/// MARK: - LES

/**
 * An Ethereum LES handles Geth LESv2 and Parity PIPv1 messages sent on the Ethereum P2P network.
 *
 * LES operates on a single network and discovers nodes using the DIS (Discovery) Protocol.  LES
 * attempts to identify a set number of nodes as neighbors - typically 100; these nodes are
 * 'available'. LES also attempts to mainset a set number of active nodes - typically 3 or 4; these
 * nodes are in a connected state and LES is receiving periodic block announcements.
 *
 * LES uses the active nodes for transaction submission.  (This is for a non-BRD_ONLY EWM mode).
 *
 * LES runs in its own thread and used events/messages and associated callbacks.
 *
 * The lesProvideXYZ functions are used by BCS to submit P2P requests to peers for processing.
 * These functions have an explicit context+callback for results; because the data is provided
 * asynchronously the context+callback needs to remain valid.
 *
 * The lesProvideXYZ functions provide a data abstracction over the specific node's light
 * ethereum protocol.  Specifically, an individual node might be a GETH (LESv2) or a Parity (PIPv1)
 * node with each type having protocol specific data structures, the lesProvideXYZ interface has
 * an abstraction of the node specfic data.
 */
struct BREthereumLESRecord {

    /** Some private key */
    BRKey key;

    /** Network */
    BREthereumNetwork network;

    /** RLP Coder */
    BRRlpCoder coder;

    BREthereumBoolean discoverNodes;
    
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
     * we can just select the first one for use */
    BRArrayOf(BREthereumNode) availableNodes;

    /** Active Nodes - a subset of `nodes` in a state of 'CONNECTED' or 'CONNECTING'.  We actively
     * `select()` on these nodes to handle send/recv needs */
    BRArrayOf(BREthereumNode) activeNodesByRoute[NUMBER_OF_NODE_ROUTES];

    /** Requests - pending, have not been provisioned to a node */
    BRArrayOf (BREthereumLESRequest) requests;

    /** Unique request identifier */
    BREthereumProvisionIdentifier requestsIdentifier;

    /** The block head, that we know about.  This is also represented in the `localEndpoint`
     * status, except that there might be a lag (block head is updated by BCS through the LES
     * interface, but the localEndpoint's status is updated as a 'safe point' in LES) */
    struct {
        BREthereumHash hash;
        uint64_t number;
        UInt256 totalDifficulty;
    } head;

    BREthereumHash genesisHash;

    /** If we handle sync or not; if not, we'll only relay transactions and won't require
     * connected nodes to SERVE_{HEADERS,BLOCK,STATE} */
    BREthereumBoolean handleSync;

    /** Thread */
    pthread_t thread;
    pthread_mutex_t lock;

    /** replace with pipe() message */
    int theTimeToQuitIsNow;
    int theTimeToCleanIsNow;
    int theTimeToUpdateBlockHeadIsNow;

    int isPendingDNSSeeds;
};

static void
lesInsertNodeAsAvailable (BREthereumLES les,
                          BREthereumNode node) {
    assert (nodeHasState(node, NODE_ROUTE_TCP, NODE_AVAILABLE));

    BREthereumBoolean inserted = ETHEREUM_BOOLEAN_FALSE;
    for (size_t index = 0; index < array_count(les->availableNodes); index++)
        // A simple, slow, linear-sort insert.
        if (ETHEREUM_COMPARISON_LT == nodeCompare(node, les->availableNodes[index])) {
            array_insert (les->availableNodes, index, node);
            inserted = ETHEREUM_BOOLEAN_TRUE;
            break;
        }
    if (ETHEREUM_BOOLEAN_IS_FALSE(inserted)) array_add (les->availableNodes, node);
}

/// Create a node for `endpoint` and add it to `les->nodes`.  If `endpoint` does not exist,
/// then do nothing.
static void
lesEnsureNodeForEndpoint (BREthereumLES les,
                          OwnershipGiven BREthereumNodeEndpoint endpoint,
                          BREthereumNodeState state,
                          BREthereumNodePriority priority,
                          BREthereumBoolean *added) {

    // Skip out if given an invalid endpoint
    if (NULL == endpoint) {
        if (NULL != added) *added = ETHEREUM_BOOLEAN_FALSE;
        return;
    }

    BREthereumHash hash = nodeEndpointGetHash(endpoint);

    pthread_mutex_lock (&les->lock);

    // Lookup an existing node.
    BREthereumNode node = BRSetGet (les->nodes, &hash);

    // If there is none, then note `added`...
    if (NULL != added) *added = AS_ETHEREUM_BOOLEAN (NULL == node);

    // ... and then actually add it.
    if (NULL == node) {
        // This endpoint is new so we'll create a node and then ...
        node = nodeCreate (priority,
                           les->network,
                           les->localEndpoint,
                           endpoint,
                           (BREthereumNodeContext) les,
                           (BREthereumNodeCallbackStatus) lesHandleStatus,
                           (BREthereumNodeCallbackAnnounce) lesHandleAnnounce,
                           (BREthereumNodeCallbackProvide) lesHandleProvision,
                           (BREthereumNodeCallbackNeighbor) lesHandleNeighbor,
                           les->handleSync);
        nodeSetStateInitial (node, NODE_ROUTE_TCP, state);

        // ... add it to 'all nodes'
        BRSetAdd(les->nodes, node);

        // .... and then, if warranted, make it available
        if (nodeHasState(node, NODE_ROUTE_TCP, NODE_AVAILABLE))
            lesInsertNodeAsAvailable(les, node);
    }
    else nodeEndpointRelease(endpoint);  // we own it; release if not passed to nodeCreate()

    pthread_mutex_unlock (&les->lock);
}

/// MARK: - DNS Seeds

#if !defined (LES_BOOTSTRAP_LCL_ONLY)
typedef struct {
    BREthereumLES les;
    const char *seed;
    BREthereumNodePriority priority;
    size_t tried;
    size_t added;
} BREthereumLESSeedContext;

static int
lesSeedQuery (BREthereumLESSeedContext *context) {
    BREthereumLES les = context->les;

    if (0 != res_init()) {
        eth_log (LES_LOG_TOPIC, "Nodes '%s' Error: res_init(): '%s' / '%s'", context->seed,
                 strerror (errno),
                 hstrerror (h_errno));
        return 1;
    }

    size_t msgDataLength = 1024 * 1024;
    u_char *msgData = calloc (1, msgDataLength + 1);

    if (NULL == msgData) {
        eth_log(LES_LOG_TOPIC, "Nodes '%s' Error: calloc", context->seed);
        return 1;
    }

    int msgCount = res_query (context->seed, ns_c_in, ns_t_txt, msgData, msgDataLength);
    if (msgCount < 0) {
        eth_log (LES_LOG_TOPIC, "Nodes '%s' Error: res_query(): '%s' / '%s'", context->seed,
                 strerror (errno),
                 hstrerror (h_errno));
        free (msgData);
        return 1;
    }

    ns_msg msg;
    ns_initparse (msgData, msgCount, &msg);
    msgCount = ns_msg_count(msg, ns_s_an);

    if (!bootstrapBRDOnly || NODE_PRIORITY_BRD == context->priority) {
        pthread_mutex_lock (&les->lock);
        for (size_t i = 0; i < msgCount; i++) {
            ns_rr rr;
            char txtRecord[1024];
            BREthereumBoolean added = ETHEREUM_BOOLEAN_FALSE;

            ns_parserr (&msg, ns_s_an, i, &rr);
            ns_sprintrr (&msg, &rr, NULL, NULL, txtRecord, sizeof(txtRecord));

            // skip everything up to 'double quote'
            strtok (txtRecord, "\"");
            // from there, everything up to 'double quote' is the enode
            char *enode = strtok (NULL, "\"");

            context->tried += 1;
            lesEnsureNodeForEndpoint(les,
                                     nodeEndpointCreateEnode(enode),
                                     (BREthereumNodeState) { NODE_AVAILABLE },
                                     context->priority,
                                     &added);
            if (ETHEREUM_BOOLEAN_IS_TRUE(added))
                context->added += 1;
        }
        pthread_mutex_unlock(&les->lock);
        eth_log(LES_LOG_TOPIC, "Nodes '%s': Found: %zu, Added: %zu", context->seed, context->tried, context->added);
    }
    else
        eth_log(LES_LOG_TOPIC, "Nodes '%s': Required BRD Only: Added: 0", context->seed);

    free (msgData);
    return 0;
}
#endif

/// MARK: - Public functions

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
           OwnershipGiven BRSetOf(BREthereumNodeConfig) configs,
           BREthereumBoolean discoverNodes,
           BREthereumBoolean handleSync) {
    
    BREthereumLES les = (BREthereumLES) calloc (1, sizeof(struct BREthereumLESRecord));
    assert (NULL != les);

    // For now, create a new, random private key that is used for communication with LES nodes.
    UInt256 secret;
    arc4random_buf(secret.u64, sizeof (secret));

    // Assign the generated private key.
    BRKeySetSecret(&les->key, &secret, 0);

    // Save the network.
    les->network = network;

    // Get our shared rlpCoder.
    les->coder = rlpCoderCreate();

    les->discoverNodes = discoverNodes;

    // Save callbacks.
    les->callbackContext = callbackContext;
    les->callbackAnnounce = callbackAnnounce;
    les->callbackStatus = callbackStatus;
    les->callbackSaveNodes = callbackSaveNodes;

    les->head.hash = headHash;
    les->head.number = headNumber;
    les->head.totalDifficulty = headTotalDifficulty;
    les->genesisHash = genesisHash;

    les->handleSync = handleSync;
    
    // Our P2P capabilities - support subprotocols: LESv2 and/or PIPv1
    BRArrayOf (BREthereumP2PCapability) capabilities;
    array_new (capabilities, 2);

#if defined (LES_SUPPORT_GETH)
    array_add (capabilities, ((BREthereumP2PCapability) { "les", LES_SUPPORT_GETH_VERSION }));
#endif

#if defined (LES_SUPPORT_PARITY)
    array_add (capabilities, ((BREthereumP2PCapability) { "pip", LES_SUPPORT_PARITY_VERSION }));
#endif

    // Create the localEndpoint
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
    nodeEndpointDefineHello (les->localEndpoint, LES_LOCAL_ENDPOINT_NAME, capabilities);
    nodeEndpointShowHello   (les->localEndpoint);

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
    nodeEndpointSetStatus (les->localEndpoint,
                           messageP2PStatusCreate (0x00,  // ignored
                                                   networkGetChainId(network),
                                                   les->head.number,
                                                   les->head.hash,
                                                   les->head.totalDifficulty,
                                                   les->genesisHash,
                                                   LES_SUPPORT_GETH_ANNOUNCE_TYPE));
    nodeEndpointShowStatus (les->localEndpoint);

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

    les->theTimeToQuitIsNow = 0;
    les->theTimeToCleanIsNow = 0;
    les->theTimeToUpdateBlockHeadIsNow = 0;

    les->isPendingDNSSeeds = 1;

#if !defined(LES_BOOTSTRAP_LCL_ONLY)
    // Identify a set of initial nodes; first, use all the endpoints provided (based on `configs`)
    eth_log (LES_LOG_TOPIC, "Nodes Provided    : %zu", (NULL == configs ? 0 : BRSetCount(configs)));
    if (NULL != configs)
        FOR_SET (BREthereumNodeConfig, config, configs)
            if (!bootstrapBRDOnly || NODE_PRIORITY_BRD == config->priority)
                lesEnsureNodeForEndpoint (les,
                                          nodeConfigCreateEndpoint (config),
                                          nodeGetPreferredState (config->state),
                                          config->priority,
                                          NULL);
#endif // !defined(LES_BOOTSTRAP_LCL_ONLY)

    if (NULL != configs) BRSetFreeAll(configs, (void (*) (void*))  nodeConfigRelease);

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

    // Release `availableNodes` - nodes themselves later
    array_free (les->availableNodes);

    // Release `activeNodesByRoute`  -nodes themselves later.
    FOR_EACH_ROUTE (route)
        array_free (les->activeNodesByRoute[route]);

    BRSetApply(les->nodes, NULL, nodeReleaseForSet);
    BRSetFree(les->nodes);

    nodeEndpointRelease (les->localEndpoint);

    requestsRelease(les->requests);

    rlpCoderRelease(les->coder);

    // requests, requestsToSend

    // TODO: NodeEnpdoint Release (to release 'hello' and 'status' messages

    pthread_mutex_unlock (&les->lock);
    pthread_mutex_destroy (&les->lock);
    free (les);
}

extern void
lesClean (BREthereumLES les) {
    if (0 == pthread_mutex_trylock (&les->lock)) {
        les->theTimeToCleanIsNow = 1;
        pthread_mutex_unlock (&les->lock);
    }
}

extern void
lesUpdateBlockHead (BREthereumLES les,
                    BREthereumHash headHash,
                    uint64_t headNumber,
                    UInt256 headTotalDifficulty) {
    pthread_mutex_lock (&les->lock);
    les->head.hash = headHash;
    les->head.number = headNumber;
    les->head.totalDifficulty = headTotalDifficulty;
    les->theTimeToUpdateBlockHeadIsNow = 1;
    pthread_mutex_unlock (&les->lock);
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

extern BREthereumNodeReference
lesGetNodePrefer (BREthereumLES les) {
    BREthereumNodeReference node = NODE_REFERENCE_NIL;
    pthread_mutex_lock (&les->lock);
    // Our preferredNode is an active TCP (P2P, ETH, LES, PIP) node at index 0;
    if (array_count(les->activeNodesByRoute[NODE_ROUTE_TCP]) > 0)
        node = (BREthereumNodeReference) les->activeNodesByRoute[NODE_ROUTE_TCP][0];
    pthread_mutex_unlock (&les->lock);
    return node;
}

extern void
lesSetNodePrefer (BREthereumLES les,
               BREthereumNodeReference nodeReference) {
    // BREthereumNode node = (BREthereumNode) nodeReference;

    pthread_mutex_lock (&les->lock);

//
//    // Only think about using `node` if it is connected.
//    FOR_CONNECTED_NODES_INDEX(les, index) {
//        if (node == les->connectedNodes[index]) {
//            if (0 != index) {
//                les->connectedNodes[index] = les->connectedNodes[0];
//                les->connectedNodes[0] = node;
//            }
//            // index != 0 or not, done.
//            break;
//        }
//    }
    pthread_mutex_unlock (&les->lock);
}

extern const char *
lesGetNodeHostname (BREthereumLES les,
                    BREthereumNodeReference node) {
    return nodeEndpointGetHostname (nodeGetRemoteEndpoint ((BREthereumNode) node));
}

/// MARK: - LES Node Callbacks

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
                         (BREthereumNodeReference) node,
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
                           (BREthereumNodeReference) node,
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
                    OwnershipGiven BREthereumProvisionResult result) {
    // Find the request, invoke the callback on result.
    // TODO: On an error, should the provision be submitted to another node?
    for (size_t index = 0; index < array_count (les->requests); index++) {
        BREthereumLESRequest *request = &les->requests[index];
        // Find the request; there must be one...
        if (result.identifier == request->provision.identifier)
            switch (result.status) {
                case PROVISION_SUCCESS:
                    // On success, invoke `request->callback`

                    // We've passed ownership of the provision, in result.  We can simply
                    // remove the request (which releases the result but we passed a copy,
                    // w/ provision and w/ provision references (to hashes, etc)).

                    request->callback (request->context,
                                       les,
                                       node,
                                       result);


                    array_rm (les->requests, index);
                    return;

                case PROVISION_ERROR: {
                    // The node failed to handle the provision.  We'll deactivate the node
                    // which will reschedule the provision with another node.
                    //
                    // We've taken ownership of the provision, we retain the provision as it will
                    // be reassigned to another node.
                    //
                    // Note that the provision might be filled with some data
                    provisionReleaseResults (&request->provision);

                    // Provide an explanation.
                    char explanation[256];
                    sprintf (explanation, "Provision Error: %s, Type: %s",
                             provisionErrorGetReasonName(result.u.error.reason),
                             provisionGetTypeName(result.type));

                    lesDeactivateNode (les, NODE_ROUTE_TCP, node, explanation);
                    return;
                }
            }
    }

    // We totally missed the request for result
    assert (0);
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
                                  NODE_PRIORITY_DIS,
                                  NULL);
    // array_free (neighbors);

    // Sometimes we receive multiple callbacks for one FIND_NEIGHBORS - how to deal?
    nodeSetDiscovered(node, ETHEREUM_BOOLEAN_TRUE);
}

/// MARK: - LES (Main) Thread

static void
lesLogNodeActivate (BREthereumLES les,
                    BREthereumNode node,
                    BREthereumNodeEndpointRoute route,
                    const char *explain,
                    const char *path) {
    char desc[128];

    BREthereumNodeState state = nodeGetState(node, route);
    eth_log (LES_LOG_TOPIC, "Conn: [ %s @ %3zu, %9s ]    %15s (%s)%s%s",
             (NODE_ROUTE_TCP == route ? "TCP" : "UDP"),
             array_count(les->activeNodesByRoute[route]),
             path,
             nodeEndpointGetHostname (nodeGetRemoteEndpoint(node)),
             nodeStateDescribe (&state, desc),
             (NULL == explain ? "" : " - "),
             (NULL == explain ? "" : explain));
}

static void
lesDeactivateNodeAtIndex (BREthereumLES les,
                          BREthereumNodeEndpointRoute route,
                          BREthereumNode node,
                          size_t index,
                          const char *explain) {
    BRArrayOf(BREthereumNode) nodes = les->activeNodesByRoute[route];

    array_rm (nodes, index);
    lesLogNodeActivate(les, node, route, explain, "<=|=>");

    // Reassign provisions back as requests if this is a TCP route
    if (NODE_ROUTE_TCP == route) {
        BRArrayOf(BREthereumProvision) provisions = nodeUnhandleProvisions(node);
        for (size_t pi = 0; pi < array_count(provisions); pi++) {
            ssize_t requestIndex = lesFindRequestForProvision (les, &provisions[pi]);
            assert (-1 != requestIndex);

            // This reestablishes the provision as needing to be assigned.
            les->requests[requestIndex].node = NULL;
        }
        array_free(provisions);
    }
}

static void
lesDeactivateNode (BREthereumLES les,
                   BREthereumNodeEndpointRoute route,
                   BREthereumNode node,
                   const char *explain) {
    BRArrayOf(BREthereumNode) nodes = les->activeNodesByRoute[route];
    for (size_t ni = 0; ni < array_count(nodes); ni++) // NodeIndex
        if (node == nodes[ni]) {
            lesDeactivateNodeAtIndex (les, route, node, ni, explain);
            // A node was deactiviated (and `nodes` modified); skip out.
            break;
        }
}

static void
lesDeactivateNodes (BREthereumLES les,
                    BREthereumNodeEndpointRoute route,
                    OwnershipKept BRArrayOf(BREthereumNode) nodesToDeactivate,
                    const char *explain) {
    for (size_t ri = 0; ri < array_count(nodesToDeactivate); ri++)
        lesDeactivateNode (les, route, nodesToDeactivate[ri], explain);
}

static void
lesHandleSelectError (BREthereumLES les,
                      int error) {
    eth_log (LES_LOG_TOPIC, "Top-Level Select Error: %s", strerror(error));

    switch (error) {
        case EAGAIN:
        case EBADF:
        case EINTR:
        case EINVAL:
            // expected errors - do something ??
            break;

        default:
            // unexpected - do what?
            break;
    }
}

#if !defined (LES_BOOTSTRAP_LCL_ONLY)
static void
lesSeedQueryAll (BREthereumLES les) {
    // Create nodes from our network seeds.
    const char **seeds = networkGetSeeds (les->network);
    size_t seedsCount  = networkGetSeedsCount(les->network);

    for (size_t i = 0; i < seedsCount; i++) {
        BREthereumLESSeedContext context = { les, seeds[i],
            (0 == i ? NODE_PRIORITY_BRD : NODE_PRIORITY_DIS),
            0, 0
        };

        lesSeedQuery(&context);
    }
}

static void
lesSeedQueryAllThreaded (BREthereumLES les) {
    pthread_t thread;

    pthread_attr_t attr;
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize (&attr, 1024 * 1024);
    pthread_create (&thread, &attr, (ThreadRoutine) lesSeedQueryAll, les);
    pthread_attr_destroy(&attr);
}
#endif

static void
lesThreadBootstrapSeeds (BREthereumLES les) {
    size_t bootstrappedEndpointsCount = 0;

#if !defined (LES_BOOTSTRAP_LCL_ONLY)
    lesSeedQueryAllThreaded(les);
#endif // !defined (LES_BOOTSTRAP_LCL_ONLY)

    // Create nodes from compiled-in nodes; this is done in case the 'seed' query fails - which
    // it currently does for Android.  Take the opportunity to add in LCL nodes, if debugging.
    struct {
        BREthereumNodeType type;
        BREthereumNodePriority priority;
        const char **enodes;
    } enodesDecl[] = {
#if defined (LES_BOOTSTRAP_LCL_ONLY)
        { NODE_TYPE_PARITY,  NODE_PRIORITY_LCL, networkGetEnodesLocal (les->network, 1) },
        { NODE_TYPE_GETH,    NODE_PRIORITY_LCL, networkGetEnodesLocal (les->network, 0) },
#elif defined (LES_BOOTSTRAP_BRD_ONLY)
        { NODE_TYPE_UNKNOWN, NODE_PRIORITY_BRD, networkGetEnodesBRD (les->network) },
#else
        { NODE_TYPE_UNKNOWN, NODE_PRIORITY_DIS, networkGetEnodesCommunity (les->network) },
#endif
        { NODE_TYPE_UNKNOWN, NODE_PRIORITY_DIS, NULL }
    };

    int useParity = 0;
    int useGeth   = 0;
#if defined (LES_SUPPORT_PARITY)
    useParity = 1;
#endif
#if defined (LES_SUPPORT_GETH)
    useGeth = 1;
#endif

    for (size_t indexDecl = 0; NULL != enodesDecl[indexDecl].enodes; indexDecl++) {
        const char **enodes = enodesDecl[indexDecl].enodes;
        for (size_t index = 0; NULL != enodes[index]; index++) {
            BREthereumBoolean added = ETHEREUM_BOOLEAN_FALSE;
            BREthereumNodeType type = enodesDecl[indexDecl].type;

            if (type == NODE_TYPE_UNKNOWN ||
                (useParity && type == NODE_TYPE_PARITY) ||
                (useGeth   && type == NODE_TYPE_GETH))
                lesEnsureNodeForEndpoint (les,
                                          nodeEndpointCreateEnode(enodes[index]),
                                          (BREthereumNodeState) { NODE_AVAILABLE },
                                          enodesDecl[indexDecl].priority,
                                          &added);

            if (ETHEREUM_BOOLEAN_IS_TRUE(added))
                bootstrappedEndpointsCount += 1;
        }
    }

    // Report on the bootstrap results.
    eth_log (LES_LOG_TOPIC, "Nodes Bootstrapped: %zu", bootstrappedEndpointsCount);
    for (size_t index = 0; index < 5 && index < array_count(les->availableNodes); index++) {
        BREthereumDISNeighborEnode enode = neighborDISAsEnode (nodeEndpointGetDISNeighbor (nodeGetRemoteEndpoint (les->availableNodes[index])), 1);
        eth_log (LES_LOG_TOPIC, "  @ %zu: %s", index, enode.chars);
    }
}

static void *
lesThread (BREthereumLES les) {
#if defined (__ANDROID__)
    pthread_setname_np (les->thread, LES_THREAD_NAME);
#else
    pthread_setname_np (LES_THREAD_NAME);
#endif
    // TODO: Don't timeout pselect(); get some 'wakeup descriptor'
    struct timespec timeout = { 0, 250000000 }; // .250 seconds

    //
    fd_set readDescriptors, writeDesciptors;
    int maximumDescriptor = -1;

    // See CORE-260: the process of finding seeds, using DNS TXT fields, can take a while.
    // So, we moved it out of lesCreate() here, in lesThread().
    if (les->isPendingDNSSeeds) {
        les->isPendingDNSSeeds = 0;
        lesThreadBootstrapSeeds(les);
     }

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
        // When an individual node times out we will attempt a PING/PONG pair.  If the node
        // responds with a PONG in a reasonable time, then we won't boot the node.  It is important
        // to keep nodes connected (they are hard to find in the first place); if a node is
        // syncing its block chain it won't produce 'announce' messages and thus will timeout
        // eventually - but when syncing it will still relay transactions.
        //
        FOR_EACH_ROUTE (route) {
            BRArrayOf(BREthereumNode) nodes = les->activeNodesByRoute[route];
            for (size_t index = 0; index < array_count(nodes); index++) {
                BREthereumNode node = nodes[index];
                BREthereumBoolean tryPing = AS_ETHEREUM_BOOLEAN (NODE_ROUTE_TCP == route &&
                                                                 nodeHasState (node, NODE_ROUTE_TCP, NODE_CONNECTED));
                if (ETHEREUM_BOOLEAN_IS_TRUE (nodeHandleTime (node, route, now, tryPing))) {
                    // Note: `nodeHandleTime()` will have disconnected.
                    array_add (nodesToRemove, node);
                    // TODO: Reassign provisions
                }
                lesDeactivateNodes(les, route, nodesToRemove, "TIMEDOUT");
                array_clear (nodesToRemove);
            }
        }
        
        //
        // Handle any/all pending requests by 'establishing a provision' in the requested node.  If
        // the requested node is not connected the request must fail.
        //
        size_t requestsToFailCount = 0;
        size_t requestsToFail [array_count (les->requests)];

        //
        // Look at every request one-by-one.  If it has not been previously handled and a node is
        // available for handling it, then handle the request's provision
        //
        for (size_t index = 0; index < array_count (les->requests); index++)

            // Only handle a reqeust if it hasn't been previously handled.
            if (NULL == les->requests[index].node) {
                BREthereumNodeReference nodeRef = les->requests[index].nodeReference;

                // We require all arbitary references to have been resolved when the
                // provision was added as a request.  An `arbitary` reference is something like
                // NODE_REFERENCE_{ANY,ALL} where the request did not specify a specific node
                assert (!NODE_REFERENCE_IS_ARBITRARY(nodeRef));

                // The request will be handled based on the `nodeReference` - if the reference is
                // 'generic' we'll get a node from `activeNodesByRoute`; otherwise we'll use the
                // specific node.

#define ACTIVE_NODE(ref)                                                 \
    (((int)(ref)) < array_count(les->activeNodesByRoute[NODE_ROUTE_TCP]) \
     ? les->activeNodesByRoute[NODE_ROUTE_TCP][(int)(ref)]               \
     : NULL)

                BREthereumNode nodeToUse = (NODE_REFERENCE_IS_GENERIC (nodeRef)
                                            ? ACTIVE_NODE (nodeRef)
                                            : (BREthereumNode) les->requests[index].nodeReference);
#undef ACTIVE_NODE

                // If `nodeToUse` is NULL, then there may be no active nodes.  We'll leave the
                // request unchanged and thus will come back to handling the request once we have
                // some active nodes.
                //
                // TODO: Consider a timeout on a request beging handled?

                if (NULL != nodeToUse && nodeHasState (nodeToUse, NODE_ROUTE_TCP, NODE_CONNECTED)) {

                    les->requests[index].node = nodeToUse;

                    // Regarding memory-management of the provision:
                    //
                    // We hold the requests[index]'s provision in requests.  In the following call
                    // we pass of copy of that provision - both provisions share memory pointers
                    // to, for example, BRArrayOf(BREthereumHash).
                    //
                    // If we pass the copy, then we might mistakenly free the shared memory
                    // pointers if we release requests[index] now.  We could 'consume' the
                    // provision to avoid holding the shared memory, but then we'd lose references
                    // needed to resubmit a failed request.
                    //
                    // We'll pass the copy and not touch the provision; thereby letting the
                    // provision callbacks, on error or success, release the shared memory.

                    // Make `node` handle `provision`.  This simply establishes the provision (by
                    // defining the messages needed to provide the data) and adding it to the
                    // node's list of provisions.  Later, we'll select() on this node to send the
                    // messages and to recv results.
                    nodeHandleProvision (les->requests[index].node,
                                         les->requests[index].provision);
                }

                // ... but if `nodeToUse` is not connected and it was explicitly requested, then
                // we must fail the request.  This is the case whereby: node 'X' announced a new
                // block; we requested bodies; the node got disconnected - literally nothing to do.
                else if (nodeToUse == (BREthereumNode) les->requests[index].nodeReference)
                    requestsToFail[requestsToFailCount++] = index;
            }

        // We've requests to fail because the requested node is not connected.  Invoke the
        // request's callback with PROVISION_ERROR.
        //
        // NOTE: `requestsToFail` holds an index - in increasing order - we need to be careful
        // about removing requests which may invalidate a saved index.  The save approach is to
        // iterate through the indices in reverse order.
        //
        // We want to fail the provision, with the callback, in the proper order...
        for (size_t index = 0; index < requestsToFailCount; index++) {
            size_t requestIndex = requestsToFail[index];
            BREthereumLESRequest *request = &les->requests[requestIndex];

            request->callback (request->context,
                               les,
                               request->nodeReference,
                               (BREthereumProvisionResult) {
                                   request->provision.identifier,
                                   request->provision.type,
                                   PROVISION_ERROR,
                                   request->provision,
                                   { .error = { PROVISION_ERROR_NODE_INACTIVE }}
                               });
        }

        // ... and then remove them in reverse order.
        for (ssize_t index = requestsToFailCount - 1; index >= 0; index--)
            array_rm (les->requests, requestsToFail[index]);

        //
        // Update the read (and write) descriptors to include nodes that are 'active' on any route.
        //
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

        pthread_mutex_unlock (&les->lock);
        int selectCount = pselect (1 + maximumDescriptor, &readDescriptors, &writeDesciptors, NULL, &timeout, NULL);
        pthread_mutex_lock (&les->lock);
        if (les->theTimeToQuitIsNow) continue;

        // We've been asked to 'clean' - which means 'reclaim memory if possible'.  We'll ask
        // all nodes to clean up; but, only the active ones will have much to do.
        if (les->theTimeToCleanIsNow) {
            eth_log (LES_LOG_TOPIC, "Cleaning%s", "");
            FOR_NODES(les, node)
                nodeClean(node);
            rlpCoderReclaim(les->coder);
            les->theTimeToCleanIsNow = 0;
        }

        // The block head has updated (presumably a fully validated block head - it must have
        // a valid total difficutly and it is non-trivial to have a valid total difficulty).  The
        // new, valid blcok head impacts our local 'status' (message).  When connecting to peer
        // nodes, we can confidently report a 'status' as the new block header - rather than as
        // the genesis block, or other checkpoint we maintain.
        if (les->theTimeToUpdateBlockHeadIsNow) {
            eth_log (LES_LOG_TOPIC, "Updating Status%s", "");

            BREthereumP2PMessageStatus status = nodeEndpointGetStatus(les->localEndpoint);
            // Nothing can be holding the localEndpoint's status directly; all 'holders' are
            // nodes holding through localEndpoint.  The status itself includes possible references
            // to allocated memory.  We modify the above status and then set is in the local
            // endpoint - this 'status' safely owns any memory references; the old status is gone.
            status.headHash = les->head.hash;
            status.headNum  = les->head.number;
            status.headTd   = les->head.totalDifficulty;
            nodeEndpointSetStatus (les->localEndpoint, status);

            // This will possibly change the state of nodes are are not available by making them
            // available and inserting them, prioritized in `availableNodes`.  Importantly,
            // `connectedNodes` does not change.
            FOR_NODES (les, node)
                if (ETHEREUM_BOOLEAN_IS_TRUE (nodeUpdatedLocalStatus(node, NODE_ROUTE_TCP)))
                    lesInsertNodeAsAvailable (les, node);

            les->theTimeToUpdateBlockHeadIsNow = 0;
        }

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

            // If we don't have enough availableNodes, try to discover some
            if (ETHEREUM_BOOLEAN_IS_TRUE(les->discoverNodes) &&
                array_count(les->availableNodes) < LES_AVAILABLE_NODES_COUNT &&
                // We won't look any more if we we have enough nodes already looking.  Upon
                // discovery, the UPD node will will go inactive and we'll look again.
                array_count(les->activeNodesByRoute[NODE_ROUTE_UDP]) < LES_ACTIVE_NODE_UDP_LIMIT) {

                // Find a 'discovery' node by looking in: activeNodesByRoute[NODE_ROUTE_TCP],
                // availableNodes and then finally allNodes.  If that fails, try harder (see
                // details in lesNodeFindDiscovery()
                BREthereumNode node = lesNodeFindDiscovery(les);

                // Try to connect...
                nodeConnect (node, NODE_ROUTE_UDP, now);

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

            // If we don't have enough connectedNodes, try to add one.  Note: when we created
            // the node (as part of UDP discovery) we give it our endpoint info (like headNum).
            // But now that is likely out of date as we've synced/progressed/chained.  I think the
            // upcoming `nodeConnect()` needs a new `status` - but how do we update the status as
            // only BCS knows where we are?

            if (array_count(les->activeNodesByRoute[NODE_ROUTE_TCP]) < LES_ACTIVE_NODE_COUNT &&
                array_count(les->availableNodes) > 0) {
                BREthereumNode node = les->availableNodes[0];

                // This blocks on Unix connect() and then loops on select() for EINPROGRESS.
                // Really, really we need NODE_CONNECT_OPEN_SOCKET_IN_PROGRESS with a small
                // timeout on connect().
                
                nodeConnect (node, NODE_ROUTE_TCP, now);

                switch (nodeGetState(node, NODE_ROUTE_TCP).type) {
                    case NODE_AVAILABLE:
                        break;

                    case NODE_ERROR:
                        // On error; no longer available
                        lesLogNodeActivate(les, node, NODE_ROUTE_TCP, "", "<=|=>");
                        array_rm (les->availableNodes, 0);

                        // TODO: Restore to 'AVAILABLE'
                        //
                        // If this node has a priority of NODE_PRIORITY_LCL or NODE_PRIORITY_BRD
                        // then consider returning it to available. See JIRA:CORE-257 - finding
                        // viable LES/PIP nodes is so rare that we simply cannot afford to
                        // eliminate options that we trust.
                        //
                        // Note: We attempted doing just the above in `nodeDisconnect()`.  With
                        // that we returned to this code and immediately retried to connect to
                        // the same node - failing again and again, forever.  Thus if we consider
                        // returning to available, at the very least we must put the node at the
                        // end of `les->availableNodes`.

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
        }

        //
        // or we have an pselect() error.
        //
        else lesHandleSelectError (les, errno);

        // double check that everything has been handled.
        assert (0 == array_count(nodesToRemove));

    } // end while (!les->theTimeToQuitIsNow)

    array_free (nodesToRemove);

    eth_log (LES_LOG_TOPIC, "Stop: Nodes: %zu, Available: %zu, Connected: [%zu, %zu]",
             BRSetCount(les->nodes),
             array_count (les->availableNodes),
             array_count (les->activeNodesByRoute[NODE_ROUTE_UDP]),
             array_count (les->activeNodesByRoute[NODE_ROUTE_TCP]));

    // Callback on Node Config.  Create the set of configs based on the current node -in
    // particular the node's state.  When we load these configs (see lesCreate()) many of the
    // node states will be remapped to 'NODE_AVAILABLE'
    BRArrayOf(BREthereumNodeConfig) configs;
    array_new (configs, BRSetCount(les->nodes));
    FOR_NODES (les, node)
        array_add (configs, nodeConfigCreate(node));
    les->callbackSaveNodes (les->callbackContext, configs);

    // Disconnect all active nodes
    FOR_EACH_ROUTE(route) {
        BRArrayOf(BREthereumNode) nodes = les->activeNodesByRoute[route];
        // In reverse as `nodes` will have `array_rm()` called
        for (size_t index = array_count(nodes); index > 0; index--)
            lesDeactivateNodeAtIndex (les, route, nodes[index - 1], index - 1, "LES Disconnect");
        array_clear(nodes);
    }

    // Available nodes...

    FOR_NODES (les, node) {
        nodeShow (node);
        nodeDisconnect (node, NODE_ROUTE_UDP, (BREthereumNodeState) { NODE_AVAILABLE }, ETHEREUM_BOOLEAN_FALSE);

        // For the TCP route, reset the node with its preferred state.  This will change many nodes
        // that are in an error state, back to NODE_AVAILABLE - when LES connects again we'll then
        // attempt to connect to that node.  That connection might be futile; we'll try anyways.
        BREthereumNodeState oldState = nodeGetState(node, NODE_ROUTE_TCP);

        // Always restore BRD and LCL nodes to 'NODE_AVAILABLE'
        BREthereumNodeState newState = (NODE_PRIORITY_BRD == nodeGetPriority(node) || NODE_PRIORITY_LCL == nodeGetPriority(node)
                                        ? ((BREthereumNodeState) { NODE_AVAILABLE })
                                        : nodeGetPreferredState(oldState));

        nodeDisconnect (node, NODE_ROUTE_TCP, newState, ETHEREUM_BOOLEAN_FALSE);
        if (NODE_AVAILABLE != oldState.type && NODE_AVAILABLE == newState.type)
            lesInsertNodeAsAvailable (les, node);
    }

    // Clear all requests - with provisions. Don't change the request identifier.  Note: don't
    // release les->requests as we need the memory at les->requests
    for (size_t index = 0; index < array_count(les->requests); index++)
        requestRelease(&les->requests[index]);
    array_clear(les->requests);

    // Something with 'head {hash, number, totalDifficulty}'?

    les->theTimeToQuitIsNow = 0;
    les->theTimeToCleanIsNow = 0;
    les->theTimeToUpdateBlockHeadIsNow = 0;

    // exit
    pthread_mutex_unlock (&les->lock);

    pthread_exit (0);
}

/// MARK: - (Public) Provide (Headers, ...)

static void
lesAddRequestSpecifically (BREthereumLES les,
                           BREthereumNodeReference node,
                           BREthereumLESProvisionContext context,
                           BREthereumLESProvisionCallback callback,
                           OwnershipGiven BREthereumProvision provision) {
    provision.identifier = les->requestsIdentifier++;
    BREthereumLESRequest request = { context, callback, provision, node, NULL };
    array_add (les->requests, request);
}

/**
 * Use `provision` it define a new LES request.  The request will be dispatched to the preferred
 * node, when appropriate.
 *
 * @param les
 * @param context
 * @param callback
 * @param provision request's provision - OwnershipGiven to LES
 */
static void
lesAddRequest (BREthereumLES les,
               BREthereumNodeReference node,
               BREthereumLESProvisionContext context,
               BREthereumLESProvisionCallback callback,
               OwnershipGiven BREthereumProvision provision) {
    assert (PROVISION_IDENTIFIER_UNDEFINED == provision.identifier);

    if (NODE_REFERENCE_NIL == node) node = NODE_REFERENCE_0;
    if (NODE_REFERENCE_ANY == node) node = NODE_REFERENCE_0;

    pthread_mutex_lock (&les->lock);
    if (NODE_REFERENCE_ALL != node)
        lesAddRequestSpecifically (les, node, context, callback, provision);
    else {
        // We'll make NODE_REFERENCE_MAX - NODE_REFERENCE_MIN specific requests.  Since we have at
        // most LES_ACTIVE_NODE_COUNT active nodes, we might not get (MAX - MIN) actual requests
        // but only as many as the number of active nodes.  See ACTIVE_NODE above (which might
        // discard node reference over the active nodes).
        for (BREthereumNodeReference ns = NODE_REFERENCE_MIN; ns <= NODE_REFERENCE_MAX; ns++)
            lesAddRequestSpecifically (les, ns, context, callback,
                                       provisionCopy (&provision, ETHEREUM_BOOLEAN_FALSE));
        // Handle `OwnershipGiven`
        provisionRelease (&provision, ETHEREUM_BOOLEAN_TRUE);
    }
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
                        BREthereumNodeReference node,
                        BREthereumLESProvisionContext context,
                        BREthereumLESProvisionCallback callback,
                        uint64_t start,  // Block Number
                        uint32_t limit,
                        uint64_t skip,
                        BREthereumBoolean reverse) {
    lesAddRequest (les, node, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_BLOCK_HEADERS,
                       { .headers = { start, skip, limit, reverse, NULL }}
                   });
}

extern void
lesProvideBlockProofs (BREthereumLES les,
                       BREthereumNodeReference node,
                       BREthereumLESProvisionContext context,
                       BREthereumLESProvisionCallback callback,
                       OwnershipGiven BRArrayOf(uint64_t) blockNumbers) {
    lesAddRequest (les, node, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_BLOCK_PROOFS,
                       { .proofs = { blockNumbers, NULL }}
                   });
}

extern void
lesProvideBlockProofsOne (BREthereumLES les,
                          BREthereumNodeReference node,
                          BREthereumLESProvisionContext context,
                          BREthereumLESProvisionCallback callback,
                          uint64_t blockNumber) {
    BRArrayOf(uint64_t) blockNumbers;
    array_new (blockNumbers, 1);
    array_add (blockNumbers, blockNumber);
    lesProvideBlockProofs (les, node, context, callback, blockNumbers);
}

extern void
lesProvideBlockBodies (BREthereumLES les,
                       BREthereumNodeReference node,
                       BREthereumLESProvisionContext context,
                       BREthereumLESProvisionCallback callback,
                       OwnershipGiven BRArrayOf(BREthereumHash) blockHashes) {
    lesAddRequest (les, node, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_BLOCK_BODIES,
                       { .bodies = { blockHashes, NULL }}
                   });
}

extern void
lesProvideBlockBodiesOne (BREthereumLES les,
                          BREthereumNodeReference node,
                          BREthereumLESProvisionContext context,
                          BREthereumLESProvisionCallback callback,
                          BREthereumHash blockHash) {
    lesProvideBlockBodies (les, node, context, callback,
                           lesCreateHashArray (les, blockHash));
}

extern void
lesProvideReceipts (BREthereumLES les,
                    BREthereumNodeReference node,
                    BREthereumLESProvisionContext context,
                    BREthereumLESProvisionCallback callback,
                    OwnershipGiven BRArrayOf(BREthereumHash) blockHashes) {
    lesAddRequest (les, node, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_TRANSACTION_RECEIPTS,
                       { .receipts = { blockHashes, NULL }}
                   });
}

extern void
lesProvideReceiptsOne (BREthereumLES les,
                       BREthereumNodeReference node,
                       BREthereumLESProvisionContext context,
                       BREthereumLESProvisionCallback callback,
                       BREthereumHash blockHash) {
    lesProvideReceipts (les, node, context, callback,
                        lesCreateHashArray(les, blockHash));
}

extern void
lesProvideAccountStates (BREthereumLES les,
                         BREthereumNodeReference node,
                         BREthereumLESProvisionContext context,
                         BREthereumLESProvisionCallback callback,
                         BREthereumAddress address,
                         OwnershipGiven BRArrayOf(BREthereumHash) blockHashes) {
    lesAddRequest (les, node, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_ACCOUNTS,
                       { .accounts = { address, blockHashes, NULL }}
                   });
}

extern void
lesProvideAccountStatesOne (BREthereumLES les,
                            BREthereumNodeReference node,
                            BREthereumLESProvisionContext context,
                            BREthereumLESProvisionCallback callback,
                            BREthereumAddress address,
                            BREthereumHash blockHash) {
    lesProvideAccountStates (les, node, context, callback, address,
                             lesCreateHashArray(les, blockHash));
}

extern void
lesProvideTransactionStatus (BREthereumLES les,
                             BREthereumNodeReference node,
                             BREthereumLESProvisionContext context,
                             BREthereumLESProvisionCallback callback,
                             OwnershipGiven BRArrayOf(BREthereumHash) transactionHashes) {
    lesAddRequest (les, node, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_TRANSACTION_STATUSES,
                       { .statuses = { transactionHashes, NULL }}
                   });
}

extern void
lesProvideTransactionStatusOne (BREthereumLES les,
                                BREthereumNodeReference node,
                                BREthereumLESProvisionContext context,
                                BREthereumLESProvisionCallback callback,
                                BREthereumHash transactionHash) {
    lesProvideTransactionStatus (les, node, context, callback,
                                 lesCreateHashArray(les, transactionHash));
}

extern void
lesSubmitTransaction (BREthereumLES les,
                      BREthereumNodeReference node,
                      BREthereumLESProvisionContext context,
                      BREthereumLESProvisionCallback callback,
                      OwnershipGiven BREthereumTransaction transaction) {
    lesAddRequest (les, node, context, callback,
                   (BREthereumProvision) {
                       PROVISION_IDENTIFIER_UNDEFINED,
                       PROVISION_SUBMIT_TRANSACTION,
                       { .submission = { transaction, {} }}
                   });
}

extern void
lesRetryProvision (BREthereumLES les,
                   BREthereumNodeReference node,
                   BREthereumLESProvisionContext context,
                   BREthereumLESProvisionCallback callback,
                   OwnershipGiven BREthereumProvision *provision) {
    provision->identifier = PROVISION_IDENTIFIER_UNDEFINED;
    lesAddRequest (les, node, context, callback, *provision);
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
