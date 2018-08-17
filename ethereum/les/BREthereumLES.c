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
#include <unistd.h>

#include "BRInt.h"
#include "BRArray.h"

#include "../rlp/BRRlp.h"
#include "../util/BRUtil.h"
#include "../base/BREthereumBase.h"

#include "BREthereumLES.h"
#include "BREthereumLESRandom.h"
#include "BREthereumLESMessage.h"
#include "BREthereumLESNode.h"

/** Forward Declarations */
static void
lesMessageHandler (BREthereumLES les,
                   BREthereumLESNode node,
                   BREthereumLESMessage message);

#define ETH_LOG_TOPIC "LAA"

#define LES_LOCAL_ENDPOINT_ADDRESS    "1.1.1.1"
#define LES_LOCAL_ENDPOINT_TCP_PORT   30303
#define LES_LOCAL_ENDPOINT_UDP_PORT   30303
#define LES_LOCAL_ENDPOINT_NAME       "BRD Light Client"

#define FOR_NODES_INDEX( les, index) \
  for (size_t index = 0; index < array_count ((les)->nodes); index++)

#define DEFAULT_UDPPORT     (30303)
#define DEFAULT_TCPPORT     (30303)

struct BREtheremLESNodeEndpointSpec {
    const char *address;
    uint32_t portUDP;
    uint32_t portTCP;
    const char *nodeId;
} bootstrapNodeEndpointSpecs[] = {
    {   // BRD
        "104.197.99.24",
        DEFAULT_UDPPORT,
        DEFAULT_TCPPORT,
        "e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851"
    },

    {   // Public GETH
        "109.232.77.21",
        DEFAULT_UDPPORT,
        DEFAULT_TCPPORT,
        "3e9301c797f3863d7d0f29eec9a416f13956bd3a14eec7e0cf5eb56942841526269209edf6f57cd1315bef60c4ebbe3476bc5457bed4e479cac844c8c9e375d3"
    },

    {   // Public GETH
        "35.226.238.26",
        DEFAULT_UDPPORT,
        DEFAULT_TCPPORT,
        "e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851"
    },

    {   // Public Parity
        "193.70.55.37",
        DEFAULT_UDPPORT,
        DEFAULT_TCPPORT,
        "81863f47e9bd652585d3f78b4b2ee07b93dad603fd9bc3c293e1244250725998adc88da0cef48f1de89b15ab92b15db8f43dc2b6fb8fbd86a6f217a1dd886701"
    }
};
#define NUMBER_OF_NODE_ENDPOINT_SPECS   (sizeof (bootstrapNodeEndpointSpecs) / sizeof (struct BREtheremLESNodeEndpointSpec))

static BREthereumLESNodeEndpoint
nodeEndpointCreateFromSpec (struct BREtheremLESNodeEndpointSpec *spec) {
    // Remote Endpoint
    BRKey key;
    key.pubKey[0] = 0x04;
    key.compressed = 0;
    decodeHex(&key.pubKey[1], 64, spec->nodeId, 128);

    return nodeEndpointCreate (spec->address, spec->portUDP, spec->portTCP, key);
}

#if 0
// Provided to Node Manager
typedef void* BREthereumSubProtoContext;
typedef void (*BREthereumSubProtoRecMsgCallback)(BREthereumSubProtoContext info, uint64_t messageType, BRRlpData messageBody);
typedef void (*BREthereumSubProtoConnectedCallback)(BREthereumSubProtoContext info, uint8_t** statusBytes, size_t* statusSize);
typedef void (*BREthereumSubProtoNetworkReachableCallback)(BREthereumSubProtoContext info, BREthereumBoolean isReachable);

typedef struct {
    BREthereumSubProtoContext info;
    BREthereumSubProtoRecMsgCallback messageRecFunc;
    BREthereumSubProtoConnectedCallback connectedFunc;
    BREthereumSubProtoNetworkReachableCallback networkReachFunc;
}BREthereumSubProtoCallbacks;
#endif

typedef struct {
    uint64_t requestId;
    BREthereumLESMessageIdentifier identifier;
    union {
        struct {
            BREthereumLESTransactionStatusCallback callback;
            BREthereumLESTransactionStatusContext context;
            BREthereumHash* transactions;
        } getTxStatus;

        struct {
            BREthereumLESBlockHeadersCallback callback;
            BREthereumLESBlockHeadersContext context;
        } getBlockHeaders;

        struct {
            BREthereumLESBlockBodiesCallback callback;
            BREthereumLESBlockBodiesContext context;
            BREthereumHash* blocks;
        } getBlockBodies;

        struct {
            BREthereumLESReceiptsCallback callback;
            BREthereumLESBlockBodiesContext context;
            BREthereumHash* blocks;
        } getReceipts;

        struct {
            BREthereumLESProofsV2Callback callback;
            BREthereumLESProofsV2Context context;
            BREthereumLESMessageGetProofsSpec *specs;
        } getProofsV2;
    } u;
} BREthereumLESReqeust;

struct BREthereumLESRecord {

    /** Some private key */
    BRKey key;

    /** Or Local Endpoint. */
    BREthereumLESNodeEndpoint localEndpoint;

    /** Array of Nodes */
    BRArrayOf(BREthereumLESNode) nodes;

    /** Callbacks */
    BREthereumLESCallbackContext callbackContext;
    BREthereumLESCallbackAnnounce callbackAnnounce;
    BREthereumLESCallbackStatus callbackStatus;

    /** Network */
    BREthereumNetwork network;

    //The random context for generating random data
    // We need this so we can assign the Nodes with random public keys
    BREthereumLESRandomContext randomContext;

    /** RLP Coder */
    BRRlpCoder coder;

    uint64_t messageRequestId;

    pthread_mutex_t lock;

    BRArrayOf (BREthereumLESReqeust) requests;
};

static BREthereumLESNodeEndpoint
createLocalEndpoint (BREthereumLESRandomContext randomContext) {
    BRKey localKey, localEphemeralKey;
    UInt256 localNonce;

    randomGenPriKey  (randomContext, &localKey);
    randomGenPriKey  (randomContext, &localEphemeralKey);
    randomGenUInt256 (randomContext, &localNonce);

    assert (0 == localKey.compressed);

    return nodeEndpointCreateRaw (LES_LOCAL_ENDPOINT_ADDRESS,
                                  LES_LOCAL_ENDPOINT_UDP_PORT,
                                  LES_LOCAL_ENDPOINT_TCP_PORT,
                                  localKey,
                                  localEphemeralKey,
                                  localNonce);
}

static void
assignLocalEndpointHelloMessage (BREthereumLESNodeEndpoint *endpoint) {
    BREthereumP2PMessage hello = {
        P2P_MESSAGE_HELLO,
        { .hello  = {
            0x03,
            strdup (LES_LOCAL_ENDPOINT_NAME),
            NULL,
            0,
        }}};

    // TODO: Limited to GETH LES v2...
    BREthereumP2PCapability lesCap = { "les", 2 };
    array_new (hello.u.hello.capabilities, 1);
    array_add (hello.u.hello.capabilities, lesCap);

    // The NodeID is the 64-byte (uncompressed) public key
    uint8_t pubKey[65];
    assert (65 == BRKeyPubKey (&endpoint->key, pubKey, 65));
    memcpy (hello.u.hello.nodeId.u8, &pubKey[1], 64);

    nodeEndpointSetHello (endpoint, hello);
}

//
// Public functions
//
extern BREthereumLES
lesCreate (BREthereumNetwork network,
           BREthereumLESCallbackContext callbackContext,
           BREthereumLESCallbackAnnounce callbackAnnounce,
           BREthereumLESCallbackStatus callbackStatus,
           BREthereumHash headHash,
           uint64_t headNumber,
           UInt256 headTotalDifficulty,
           BREthereumHash genesisHash) {
    
    BREthereumLES les = (BREthereumLES) calloc(1,sizeof(struct BREthereumLESRecord));
    assert (NULL != les);

    // For now, create a new, random private key that is used for communication with LES nodes.
    UInt256 secret;
#if defined (__ANDROID__)
#else
    arc4random_buf(secret.u64, sizeof (secret));
#endif

    // Assign the generated private key.
    BRKeySetSecret(&les->key, &secret, 0);
    // Previously the above key was generated with:
#if 0
    les->key = (BRKey *)malloc(sizeof(BRKey));
    uint8_t data[32] = {3,4,5,0};
    memcpy(les->key->secret.u8,data,32);
    les->key->compressed = 0;
#endif

    // Save the network.
    les->network = network;

    // Use the privateKey to create a randomContext
    les->randomContext = randomCreate(les->key.secret.u8, 32);

    // Create a local endpoint; when creating nodes we'll use this local endpoint repeatedly.
    les->localEndpoint = createLocalEndpoint(les->randomContext);

    // The 'hello' message is fixed; assign it to the local endpoint
    assignLocalEndpointHelloMessage(&les->localEndpoint);

    // The 'status' message MIGHT BE fixed; create one and assign it to the local endoint.
    // TODO: Should we update the status message every time we sync up with a new head?
    // TODO: Can we appears as a GETH LES v2 or as a PARITY PIP peer?
    BREthereumLESMessage status = {
        LES_MESSAGE_STATUS,
        { .status = messageLESStatusCreate (0x02,  // LES v2
                                            networkGetChainId(network),
                                            headNumber,
                                            headHash,
                                            headTotalDifficulty,
                                            genesisHash,
                                            0x01) // Announce type (of LES v2)
        }};
    nodeEndpointSetStatus(&les->localEndpoint, status);

    // Get our shared rlpCoder.
    les->coder = rlpCoderCreate();

    les->callbackContext = callbackContext;
    les->callbackAnnounce = callbackAnnounce;
    les->callbackStatus = callbackStatus;
    
    // Create the PTHREAD LOCK variable
    {
        // The cacheLock is a normal, non-recursive lock
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); // overkill, but needed still
        pthread_mutex_init(&les->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    // TODO: message Request (Set)
    array_new (les->requests, 10);

    //
    les->messageRequestId = 0;

    // Fill in our bootstrap nodes
    array_new (les->nodes, 10);

    BREthereumLESNodeEndpoint nodeEndpoint = nodeEndpointCreateFromSpec (&bootstrapNodeEndpointSpecs[0]);
    BREthereumLESNode node = nodeCreate (nodeEndpoint,
                                         les->localEndpoint,
                                         (BREthereumLESNodeContext) les,
                                         (BREthereumLESNodeCallbackMessage) lesMessageHandler);
    array_add(les->nodes, node);

    return les;
}

extern void
lesStart (BREthereumLES les) {
    FOR_NODES_INDEX(les, index)
    nodeStart (les->nodes[index]);
}

extern void
lesStop (BREthereumLES les) {
    pthread_mutex_lock (&les->lock);
    FOR_NODES_INDEX(les, index)
        nodeStop(les->nodes[index]);
    pthread_mutex_unlock (&les->lock);
}

extern void lesRelease(BREthereumLES les) {
    pthread_mutex_lock (&les->lock);
    lesStop (les);

    FOR_NODES_INDEX(les, index) {
        nodeRelease(les->nodes[index]);
        array_rm (les->nodes, index);
    }
    array_free (les->nodes);

    randomRelease(les->randomContext);
    rlpCoderRelease(les->coder);

    // TODO: NodeEnpdoint Release (to release 'hello' and 'status' messages

    pthread_mutex_unlock (&les->lock);
    pthread_mutex_destroy (&les->lock);
    free (les);
}

static uint64_t
lesGetThenIncRequestId (BREthereumLES les) {
    uint64_t requestId;
    pthread_mutex_lock(&les->lock);
    requestId = les->messageRequestId++;
    pthread_mutex_unlock(&les->lock);
    return requestId;
}

static void
lesSendMessage (BREthereumLES les,
                BREthereumMessage message) {

    // TODO: Node Send will write to socketTCP - don't mixup writes from multiple threads
    pthread_mutex_lock(&les->lock);
    FOR_NODES_INDEX(les, index) {
        if (1) // should send (is TCP, is connected
            nodeSend(les->nodes[index], message);
    }
    pthread_mutex_unlock(&les->lock);
}

static void
lesAddReqeust (BREthereumLES les,
               uint64_t requestId,
               void *context,
               void (*callback) (),
               BREthereumMessage message) {
    BREthereumLESReqeust request;

    switch (message.u.les.identifier) {
        case LES_MESSAGE_GET_BLOCK_HEADERS:
            request = (BREthereumLESReqeust) {
                requestId,
                LES_MESSAGE_GET_BLOCK_HEADERS,
                { .getBlockHeaders = {
                    (BREthereumLESBlockHeadersCallback) callback,
                    (BREthereumLESBlockHeadersContext) context }}
            };
            break;

        case LES_MESSAGE_GET_BLOCK_BODIES:
            request = (BREthereumLESReqeust) {
                requestId,
                LES_MESSAGE_GET_BLOCK_BODIES,
                { .getBlockBodies = {
                    (BREthereumLESBlockBodiesCallback) callback,
                    (BREthereumLESBlockBodiesContext) context,
                    message.u.les.u.getBlockBodies.hashes }}
            };
            break;

        case LES_MESSAGE_GET_RECEIPTS:
            request = (BREthereumLESReqeust) {
                requestId,
                LES_MESSAGE_GET_RECEIPTS,
                { .getReceipts = {
                    (BREthereumLESReceiptsCallback) callback,
                    (BREthereumLESReceiptsContext) context,
                    message.u.les.u.getReceipts.hashes }}
            };
            break;

        case LES_MESSAGE_GET_TX_STATUS:
            request = (BREthereumLESReqeust) {
                requestId,
                LES_MESSAGE_GET_TX_STATUS,
                { .getTxStatus = {
                    (BREthereumLESTransactionStatusCallback) callback,
                    (BREthereumLESTransactionStatusContext) context,
                    message.u.les.u.getTxStatus.hashes }}
            };
            break;

//        case LES_MESSAGE_GET_PROOFS:
//            request = (BREthereumLESReqeust) {
//                requestId,
//                LES_MESSAGE_GET_PROOFS,
//                { .getProofsV2 = {
//                    (BREthereumLESProofsV2Callback) callback,
//                    (BREthereumLESProofsV2Context) context,
//                    message.u.les.u.getProofs.specs }}
//            };
//            break;

        case LES_MESSAGE_GET_PROOFS_V2:
            request = (BREthereumLESReqeust) {
                requestId,
                LES_MESSAGE_GET_PROOFS_V2,
                { .getProofsV2 = {
                    (BREthereumLESProofsV2Callback) callback,
                    (BREthereumLESProofsV2Context) context,
                    message.u.les.u.getProofs.specs }}
            };
            break;

        default:
            assert (0);
            break;
    }

    array_add (les->requests, request);
}

static long
lesLookupRequest (BREthereumLES les,
                  uint64_t requestId) {
    long result = -1;
    pthread_mutex_lock(&les->lock);
    for (size_t index = 0; index < array_count (les->requests); index++)
        if (requestId == les->requests[index].requestId) {
            result = index;
            break;
        }
    pthread_mutex_unlock(&les->lock);
    return result;
}

/// MARK: Get Block Headers

extern BREthereumLESStatus
lesGetBlockHeaders (BREthereumLES les,
                    BREthereumLESBlockHeadersContext context,
                    BREthereumLESBlockHeadersCallback callback,
                    uint64_t blockNumber,
                    uint32_t maxBlockCount,
                    uint64_t skip,
                    BREthereumBoolean reverse) {
    uint64_t requestId = lesGetThenIncRequestId (les);

    BREthereumMessage message = {
        MESSAGE_LES,
        { .les = {
            LES_MESSAGE_GET_BLOCK_HEADERS,
            { .getBlockHeaders = messageLESGetBlockHeadersCreate (requestId,
                                                                  blockNumber,
                                                                  maxBlockCount,
                                                                  skip,
                                                                  ETHEREUM_BOOLEAN_IS_TRUE (reverse)) }}}
    };

    // Add to requests - {context, callback, requestId
    lesAddReqeust(les, requestId, context, callback, message);

    // Send the message
    lesSendMessage (les, message);

    return LES_SUCCESS;
}

/// MARK: Get Block Bodies

extern BREthereumLESStatus
lesGetBlockBodies (BREthereumLES les,
                   BREthereumLESBlockBodiesContext context,
                   BREthereumLESBlockBodiesCallback callback,
                   BRArrayOf(BREthereumHash) hashes) {
    uint64_t requestId = lesGetThenIncRequestId (les);

    BREthereumMessage message = {
        MESSAGE_LES,
        { .les = {
            LES_MESSAGE_GET_BLOCK_BODIES,
            { .getBlockBodies = { requestId, hashes }}}}
    };

    // Add to requests - {context, callback, requestId
    lesAddReqeust(les, requestId, context, callback, message);

    // Send the message
    lesSendMessage (les, message);

    return LES_SUCCESS;
}

extern BREthereumLESStatus
lesGetBlockBodiesOne (BREthereumLES les,
                      BREthereumLESBlockBodiesContext context,
                      BREthereumLESBlockBodiesCallback callback,
                      BREthereumHash hash) {
    BRArrayOf(BREthereumHash) hashes;
    array_new (hashes, 1);
    array_add (hashes, hash);
    return lesGetBlockBodies (les, context, callback, hashes);
}

/// MARK: Get Receipts

extern BREthereumLESStatus
lesGetReceipts (BREthereumLES les,
                BREthereumLESReceiptsContext context,
                BREthereumLESReceiptsCallback callback,
                BRArrayOf(BREthereumHash) hashes) {
    uint64_t requestId = lesGetThenIncRequestId (les);

    BREthereumMessage message = {
        MESSAGE_LES,
        { .les = {
            LES_MESSAGE_GET_RECEIPTS,
            { .getReceipts = { requestId, hashes }}}}
    };

    // Add to requests - {context, callback, requestId
    lesAddReqeust(les, requestId, context, callback, message);

    // Send the message
    lesSendMessage (les, message);

    return LES_SUCCESS;
}

extern BREthereumLESStatus
lesGetReceiptsOne (BREthereumLES les,
                   BREthereumLESReceiptsContext context,
                   BREthereumLESReceiptsCallback callback,
                   BREthereumHash hash) {

    BRArrayOf(BREthereumHash) hashes;
    array_new (hashes, 1);
    array_add (hashes, hash);
    return lesGetReceipts (les, context, callback, hashes);
}

/// MARK: Get Account State

extern void
lesGetAccountState (BREthereumLES les,
                    BREthereumLESAccountStateContext context,
                    BREthereumLESAccountStateCallback callback,
                    uint64_t blockNumber,
                    BREthereumHash blockHash,
                    BREthereumAddress address) {
    // For Parity:
    //    // Request for proof of specific account in the state.
    //    Request::Account {
    //    ID: 5
    //    Inputs:
    //        Loose(H256) // block hash
    //        Loose(H256) // address hash
    //    Outputs:
    //        [U8](U8) // merkle inclusion proof from state trie
    //        U // nonce
    //        U // balance
    //        H256 reusable_as(0) // code hash
    //        H256 reusable_as(1) // storage root
    //    }

    // For GETH:
    //    Use GetProofs, then process 'nodes'

    struct BlockStateMap {
        uint64_t number;
        BREthereumAccountState state;
    };

    // Address: 0xa9de3dbD7d561e67527bC1Ecb025c59D53b9F7Ef
    static struct BlockStateMap map[] = {
        { 0, { 0 }},
        { 5506602, { 1 }}, // <- ETH, 0xa9d8724bb9db4b5ad5a370201f7367c0f731bfaa2adf1219256c7a40a76c8096
        { 5506764, { 2 }}, // -> KNC, 0xaca2b09703d7816753885fd1a60e65c6426f9d006ba2d8dd97f7c845e0ffa930
        { 5509990, { 3 }}, // -> KNC, 0xe5a045bdd432a8edc345ff830641d1b75847ab5c9d8380241323fa4c9e6cee1e
        { 5511681, { 4 }}, // -> KNC, 0x04d93a1addec69da4a0589bd84d5157a0b47369ce6084c06d66fbd0afc8591dc
        { 5539808, { 5 }}, // -> KNC, 0x932faac9e5bf5cead0492afbe290ff0cd7d2ab5d7b351ad1bccae8aac646522b
        { 5795662, { 6 }}, // -> ETH, 0x1429c28066e3e41073e7abece864e5ca9b0dfcef28bec90a83e6ed04d91997ac
        { 5818087, { 7 }}, // -> ETH, 0xe606358c10f59dfbdb7ad823826881ee3915e06320f1019187af92e96201e7ed
        { 5819543, { 8 }}, // -> ETH, 0x597595bdf79ec29e8a7079fecddd741a40471bbd8fd92e11cdfc0d78d973cb16
        { 6104163, { 9 }}, // -> ETH, 0xe87d76e5a47600f70ee11816ba8d1756b9295eca12487cbe1223a80e3a603d44
        { UINT64_MAX, { 9 }}
    };

    BREthereumLESAccountStateResult result = { ACCOUNT_STATE_ERROR_X };

    for (int i = 0; UINT64_MAX != map[i].number; i++)
        if (blockNumber < map[i].number) {
            result.status = ACCOUNT_STATE_SUCCCESS;
            result.u.success.block = blockHash;
            result.u.success.address = address;
            result.u.success.accountState = map [i - 1].state;
            break;
        }

    callback (context, result);
}

/// MARK: Get Proofs V2

extern BREthereumLESStatus
lesGetProofsV2One (BREthereumLES les,
                      BREthereumLESProofsV2Context context,
                      BREthereumLESProofsV2Callback callback,
                      BREthereumHash blockHash,
                      BRRlpData key1,
                      BRRlpData key2,
                      //                     BREthereumHash  key,
                      //                     BREthereumHash key2,
                      uint64_t fromLevel) {

    uint64_t requestId = lesGetThenIncRequestId (les);

    BREthereumLESMessageGetProofsSpec spec = {
        blockHash,
        key1,
        key2,
        fromLevel
    };

    BRArrayOf(BREthereumLESMessageGetProofsSpec) specs;
    array_new (specs, 1);
    array_add (specs, spec);

    // Acutally NOT V2, BUT V1 - for now
    BREthereumMessage message = {
        MESSAGE_LES,
        { .les = {
            LES_MESSAGE_GET_PROOFS_V2,
            { .getProofs = { requestId, specs }}}}
    };

    // Add to requests - {context, callback, requestId
    lesAddReqeust(les, requestId, context, callback, message);

    // Send the message
    lesSendMessage (les, message);

    return LES_SUCCESS;
}

/// MARK: Get Transaction Status

extern BREthereumLESStatus
lesGetTransactionStatus (BREthereumLES les,
                         BREthereumLESTransactionStatusContext context,
                         BREthereumLESTransactionStatusCallback callback,
                         BRArrayOf(BREthereumHash) transactions) {
    uint64_t requestId = lesGetThenIncRequestId (les);

    BREthereumMessage message = {
        MESSAGE_LES,
        { .les = {
            LES_MESSAGE_GET_TX_STATUS,
            { .getTxStatus = { requestId, transactions }}}}
    };

    // Add to requests - {context, callback, requestId
    lesAddReqeust(les, requestId, context, callback, message);

    // Send the message
    lesSendMessage (les, message);

    return LES_SUCCESS;
}

extern BREthereumLESStatus
lesGetTransactionStatusOne (BREthereumLES les,
                            BREthereumLESTransactionStatusContext context,
                            BREthereumLESTransactionStatusCallback callback,
                            BREthereumHash transaction) {

    BRArrayOf(BREthereumHash) transactions;
    array_new (transactions, 1);
    array_add (transactions, transaction);
    return lesGetTransactionStatus (les, context, callback, transactions);
}

/// MARK: Submit Transactions

static BREthereumLESStatus
lesSubmitTransactions (BREthereumLES les,
                      BREthereumLESTransactionStatusContext context,
                      BREthereumLESTransactionStatusCallback callback,
                      BRArrayOf (BREthereumTransaction) transactions) {
    uint64_t requestId = lesGetThenIncRequestId (les);

    // TODO: Assumes LES v2
    BREthereumMessage message = {
        MESSAGE_LES,
        { .les = {
            LES_MESSAGE_SEND_TX2,
            { .sendTx2 = { requestId, transactions }}}}
    };

    // Add to requests - {context, callback, requestId
    lesAddReqeust(les, requestId, context, callback, message);

    // Send the message
    lesSendMessage (les, message);

    return LES_SUCCESS;
}

extern BREthereumLESStatus
lesSubmitTransaction (BREthereumLES les,
                      BREthereumLESTransactionStatusContext context,
                      BREthereumLESTransactionStatusCallback callback,
                      BREthereumTransaction transaction) {
    BRArrayOf(BREthereumTransaction) transactions;
    array_new (transactions, 1);
    array_add (transactions, transaction);
    return lesSubmitTransactions (les, context, callback, transactions);
}


/// MARK: Handle Messages

static void
lesMessageHandler (BREthereumLES les,
                   BREthereumLESNode node,
                   BREthereumLESMessage message) {

    switch (message.identifier) {
        case LES_MESSAGE_STATUS:
            les->callbackStatus (les->callbackContext,
                                 message.u.status.headHash,
                                 message.u.status.headNum);
            break;

        case LES_MESSAGE_ANNOUNCE:
            les->callbackAnnounce (les->callbackContext,
                                   message.u.announce.headHash,
                                   message.u.announce.headNumber,
                                   message.u.announce.headTotalDifficulty,
                                   message.u.announce.reorgDepth);
            break;

        default: {
            // look up the request
            // TODO: Wrong access to reqId, but works
            long requestIndex = lesLookupRequest(les, message.u.getBlockBodies.reqId);
            if (requestIndex == -1) return;

            BREthereumLESReqeust request = les->requests[requestIndex];

            // invoke the callback
            switch (request.identifier) {
                case LES_MESSAGE_GET_BLOCK_HEADERS:
                    if (LES_MESSAGE_BLOCK_HEADERS == message.identifier) {
                        for (size_t index = 0; index < array_count (message.u.blockHeaders.headers); index++)
                            request.u.getBlockHeaders.callback (request.u.getBlockHeaders.context,
                                                                message.u.blockHeaders.headers[index]);
                    }
                    break;

                case LES_MESSAGE_GET_BLOCK_BODIES:
                    if (LES_MESSAGE_BLOCK_BODIES == message.identifier) {
                        for (size_t index = 0; index < array_count (message.u.blockBodies.pairs); index++) {
                            BREthereumBlockBodyPair pair = message.u.blockBodies.pairs[index];
                            request.u.getBlockBodies.callback (request.u.getBlockBodies.context,
                                                               request.u.getBlockBodies.blocks[index],
                                                               pair.transactions,
                                                               pair.uncles);
                        }
                    }
                    break;

                case LES_MESSAGE_GET_RECEIPTS:
                    if (LES_MESSAGE_RECEIPTS == message.identifier) {
                        for (size_t index = 0; index < array_count (message.u.receipts.arrays); index++) {
                            BREthereumLESMessageReceiptsArray array = message.u.receipts.arrays[index];
                            request.u.getReceipts.callback (request.u.getReceipts.context,
                                                            request.u.getReceipts.blocks[index],
                                                            array.receipts);
                        }
                    }
                    break;

                case LES_MESSAGE_GET_TX_STATUS:
                    if (LES_MESSAGE_TX_STATUS == message.identifier) {
                        assert (array_count (request.u.getTxStatus.transactions) == array_count (message.u.txStatus.stati));
                        for (size_t index = 0; index < array_count (message.u.txStatus.stati); index++)
                            request.u.getTxStatus.callback (request.u.getTxStatus.context,
                                                            request.u.getTxStatus.transactions[index],
                                                            message.u.txStatus.stati[index]);
                    }
                    break;

                case LES_MESSAGE_GET_PROOFS_V2:
                    if (LES_MESSAGE_PROOFS_V2 == message.identifier) {
                        assert (array_count(request.u.getProofsV2.specs) == array_count(message.u.proofs.paths));
                        for (size_t index = 0; index < array_count (message.u.proofs.paths); index++) {
                            BREthereumLESMessageGetProofsSpec spec = request.u.getProofsV2.specs[index];
                            request.u.getProofsV2.callback (request.u.getProofsV2.context,
                                                            spec.blockHash,
                                                            spec.key1,
                                                            spec.key2);
                        }
                    }
                    break;

                default:
                    break;
            }

            array_rm (les->requests, requestIndex);
        }
            break;
    }
}

//BREthereumBoolean _findPeers(BREthereumLESNodeManager manager) {
//
//    //Note: This function should be called from within the lock of the les context
//    //For testing purposes, we will only connect to our remote node known
//    BREthereumBoolean ret = ETHEREUM_BOOLEAN_FALSE;
//
//    if(array_count(manager->connectedNodes) == 0)
//    {
//        BREthereumLESPeerConfig config;
//        uint8_t pubKey[64];
//
//#if 0 // 65...
//        config.endpoint = endpointCreate(ETHEREUM_BOOLEAN_TRUE, "65.79.142.182", 30303, 30303);
//        decodeHex (pubKey, 64, "c7f12332d767c12888da45044581d30de5a1bf383f68ef7b79c83eefd99c82adf2ebe3f37e472cbcdf839d52eddc34f270a7a3444ab6c1dd127bba1687140d93", 128);
//#elif 0 // 104...
//        config.endpoint = endpointCreate(ETHEREUM_BOOLEAN_TRUE, "104.197.99.24", DEFAULT_TCPPORT, DEFAULT_UDPPORT);
//        decodeHex (pubKey, 64, "e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851", 128);
//#elif 0 // TestNet: 35...198
//        config.endpoint = endpointCreate(ETHEREUM_BOOLEAN_TRUE, "35.226.161.198", 30303, 30303); //TestNet
//        decodeHex (pubKey, 64, "9683a29b13c7190cfd63cccba4bcb62d7b710da0b1c4bff2c4b8bcf129127d7b3f591163a58449d7f66200db3c208d06b9e9a8bea69be4a72e1728a83d703063", 128);
//#elif 0   // 35...
//        config.endpoint = endpointCreate(ETHEREUM_BOOLEAN_TRUE, "35.184.255.33", 30303, 30303);
//        decodeHex (pubKey, 64, "3d0bce4775635c65733b7534f1bccd48720632f5d66a44030c1d13e2e5883262d9d22cdb8365c03137e8d5fbbf5355772acf35b08d6f9b5ad69bb24ad52a20cc", 128);
//#elif 0  // public
//         // admin.addPeer("enode://3e9301c797f3863d7d0f29eec9a416f13956bd3a14eec7e0cf5eb56942841526269209edf6f57cd1315bef60c4ebbe3476bc5457bed4e479cac844c8c9e375d3@109.232.77.21:30303");
//        config.endpoint = endpointCreate(ETHEREUM_BOOLEAN_TRUE, "109.232.77.21", 30303, 30303);
//        decodeHex (pubKey, 64, "3e9301c797f3863d7d0f29eec9a416f13956bd3a14eec7e0cf5eb56942841526269209edf6f57cd1315bef60c4ebbe3476bc5457bed4e479cac844c8c9e375d3", 128);
//#elif 1  // public
//         //admin.addPeer("enode://e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851@35.226.238.26:30303");
//        config.endpoint = endpointCreate(ETHEREUM_BOOLEAN_TRUE, "35.226.238.26", 30303, 30303);
//        decodeHex (pubKey, 64, "e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851", 128);
//#elif 0 // parity - maybe these should work, but don't
//        // enode://81863f47e9bd652585d3f78b4b2ee07b93dad603fd9bc3c293e1244250725998adc88da0cef48f1de89b15ab92b15db8f43dc2b6fb8fbd86a6f217a1dd886701@193.70.55.37:30303
//        // enode://4afb3a9137a88267c02651052cf6fb217931b8c78ee058bb86643542a4e2e0a8d24d47d871654e1b78a276c363f3c1bc89254a973b00adc359c9e9a48f140686@144.217.139.5:30303
//        // enode://c16d390b32e6eb1c312849fe12601412313165df1a705757d671296f1ac8783c5cff09eab0118ac1f981d7148c85072f0f26407e5c68598f3ad49209fade404d@139.99.51.203:30303
//        // enode://029178d6d6f9f8026fc0bc17d5d1401aac76ec9d86633bba2320b5eed7b312980c0a210b74b20c4f9a8b0b2bf884b111fa9ea5c5f916bb9bbc0e0c8640a0f56c@216.158.85.185:30303
//        // enode://fdd1b9bb613cfbc200bba17ce199a9490edc752a833f88d4134bf52bb0d858aa5524cb3ec9366c7a4ef4637754b8b15b5dc913e4ed9fdb6022f7512d7b63f181@212.47.247.103:30303
//
//        config.endpoint = endpointCreate(ETHEREUM_BOOLEAN_TRUE, "193.70.55.37", 30303, 30303);
//        decodeHex (pubKey, 64, "81863f47e9bd652585d3f78b4b2ee07b93dad603fd9bc3c293e1244250725998adc88da0cef48f1de89b15ab92b15db8f43dc2b6fb8fbd86a6f217a1dd886701", 128);
//#elif 0 // Ed's Local Parity
//        // enode://8ebe6a85d46737451c8bd9423f37dcb117af7316bbce1643856feeaf9f81a792ff09029e9ab1796b193eb477f938af3465f911574c57161326b71aaf0221f341@192.168.1.105:30303
//        config.endpoint = endpointCreate(ETHEREUM_BOOLEAN_TRUE, "192.168.1.105", 30303, 30303);
//        decodeHex (pubKey, 64, "8ebe6a85d46737451c8bd9423f37dcb117af7316bbce1643856feeaf9f81a792ff09029e9ab1796b193eb477f938af3465f911574c57161326b71aaf0221f341", 128);
//#endif
//
//        BRKey* remoteKey = malloc(sizeof(BRKey));
//        remoteKey->pubKey[0] = 0x04;
//        memcpy(&remoteKey->pubKey[1], pubKey, 64);
//        remoteKey->compressed = 0;
//        config.remoteKey = remoteKey;
//        array_add(manager->peers, config);
//        ret = ETHEREUM_BOOLEAN_TRUE;
//    }
//
//    return ret;
//}
//
//
//int nodeManagerConnect(BREthereumLESNodeManager manager) {
//    assert(manager != NULL);
//#if 1
//    {
//        // Remote Endpoint
//        BRKey key;
//        key.pubKey[0] = 0x04;
//        key.compressed = 0;
//#if 0
//        decodeHex(&key.pubKey[1], 64, "e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851", 128);
//        BREthereumLESNodeEndpoint remote = nodeEndpointCreate("104.197.99.24", DEFAULT_UDPPORT, DEFAULT_TCPPORT, key);
//#elif 1
//        decodeHex(&key.pubKey[1], 64, "e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851", 128);
//        BREthereumLESNodeEndpoint remote = nodeEndpointCreate("35.226.238.26", 30303, 30303, key);
//#elif 0
//        decodeHex(&key.pubKey[1], 64, "a40437d2f44ae655387009d1d69ba9fd07b748b7a6ecfc958c135008a34c0497466db35049c36c8296590b4bcf9b9058f9fa2a688a2c6566654b1f1dc42417e4", 128);
//        BREthereumLESNodeEndpoint remote = nodeEndpointCreate("127.0.0.1", 30303, 30303, key);
//
//#endif
//        //
//        // Local Endpoint
//        //
//        BRKey localKey, localEphemeralKey;
//        UInt256 localNonce;
//
//        randomGenPriKey(manager->randomContext, &localKey);
//        randomGenPriKey(manager->randomContext, &localEphemeralKey);
//        randomGenUInt256(manager->randomContext, &localNonce);
//
//        BREthereumLESNodeEndpoint local  = nodeEndpointCreateRaw ("1.1.1.1", 30303, 30303,
//                                                                  localKey,
//                                                                  localEphemeralKey,
//                                                                  localNonce);
//
//        {
//            BREthereumP2PMessage hello = {
//                P2P_MESSAGE_HELLO,
//                { .hello  = {
//                    0x03,
//                    strdup ("BRD Light Client"),
//                    NULL,
//                    0,
//                }}};
//            BREthereumP2PCapability lesCap = { "les", 2 };
//            array_new (hello.u.hello.capabilities, 1);
//            array_add (hello.u.hello.capabilities, lesCap);
//
//            assert (0 == localKey.compressed);
//            uint8_t pubKey[65];
//            assert (65 == BRKeyPubKey (&localKey, pubKey, 65));
//            memcpy (hello.u.hello.nodeId.u8, &pubKey[1], 64);
//
//            nodeEndpointSetHello(&local, hello);
//        }
//
//        {
//            BREthereumLESMessage status = {
//                LES_MESSAGE_STATUS,
//                { .status = messageLESStatusCreate (0x02,
//                                                    networkGetChainId(manager->network),
//                                                    manager->headNumber,
//                                                    manager->headHash,
//                                                    manager->headTotalDifficulty,
//                                                    manager->genesisHash,
//                                                    0x01)
//                }};
//            nodeEndpointSetStatus(&local, status);
//        }
//
//        const char *dataString = "1c83e962b029ab51272b38ed7783cb3d06fbea4536dc4d8d5c4c8b40908d3b6d7259855f2ba9e7152bba05cab99196dbef8aa2b659daf7a94ef7cb6d81f0e1444401de04cb840101010182765f82765fcb847f00000182765f82765f845b8550";
//        size_t dataSize;
//        uint8_t *dataBytes = decodeHexCreate(&dataSize, dataString, strlen (dataString));
//        BRRlpData data = { dataSize, dataBytes };
//        BREthereumHash hash = hashCreateFromData(data);
//        printf ("Hash: %s\n", encodeHexCreate(&dataSize, hash.bytes, 32));
//
//        //
//        // Actual Node
//        //
//        BREthereumLESNodeX nodeX = nodeXCreate(remote, local, manager, NULL);
//        nodeXStart(nodeX);
//    }
//
//    sleep (10 * 60);
//
//    return 0;
//#endif
//    pthread_mutex_lock(&manager->lock);
//    int retValue = 0;
//    int connectedCount = 0;
//    if(array_count(manager->connectedNodes) < ETHEREUM_PEER_MAX_CONNECTIONS)
//    {
//        if(ETHEREUM_BOOLEAN_IS_TRUE(_findPeers(manager))){
//
//            int peerIdx = 0;
//            while(array_count(manager->peers) > 0 && array_count(manager->connectedNodes) < ETHEREUM_PEER_MAX_CONNECTIONS)
//            {
//                BRKey* nodeKey = (BRKey*)calloc(1,sizeof(BRKey));
//                BRKey* ephemeralKey = (BRKey*)calloc(1,sizeof(BRKey));
//                UInt256* nonce = (UInt256*)calloc(1,sizeof(UInt256));
//
//                randomGenPriKey(manager->randomContext, nodeKey);
//                randomGenPriKey(manager->randomContext, ephemeralKey);
//                randomGenUInt256(manager->randomContext, nonce);
//
//                BREthereumLESNode node = nodeCreate(manager->peers[peerIdx], nodeKey, nonce, ephemeralKey, manager->managerCallbacks, ETHEREUM_BOOLEAN_TRUE);
//                if(nodeConnect(node))
//                {
//                    //We could not connect to the remote peer so free the memory
//                    nodeRelease(node);
//                }
//                else
//                {
//                    connectedCount++;
//                    array_add(manager->connectedNodes, node);
//                }
//                array_rm(manager->peers, peerIdx);
//            }
//        }
//        else
//        {
//            eth_log(ETH_LOG_TOPIC, "%s", "Could not find any remote peers to connect to");
//            retValue = 1;
//        }
//    }
//    pthread_mutex_unlock(&manager->lock);
//
//    if(connectedCount <= 0) {
//        eth_log(ETH_LOG_TOPIC, "%s", "Could not succesfully open a connection to any remote peers");
//        retValue = 1;
//    }
//    return retValue;
//}
//
//static void _addTxtStatusRecord(
//                                BREthereumLES les,
//                                BREthereumLESTransactionStatusContext context,
//                                BREthereumLESTransactionStatusCallback callback,
//                                BREthereumHash transactions[],
//                                uint64_t reqId) {
//
//    LESRequestRecord record;
//    record.requestId = reqId;
//    record.u.transaction_status.callback = callback;
//    record.u.transaction_status.ctx = context;
//    record.u.transaction_status.transactions = malloc(sizeof(BREthereumHash) * array_count(transactions));
//    record.u.transaction_status.transactionsSize = array_count(transactions);
//    for(int i = 0; i < array_count(transactions); ++i) {
//        memcpy(record.u.transaction_status.transactions[i].bytes, transactions[i].bytes, sizeof(transactions[0].bytes));
//    }
//
//    pthread_mutex_unlock(&les->lock);
//    array_add(les->requests, record);
//    pthread_mutex_unlock(&les->lock);
//}




// Message Handler



//static int _findRequestId(BREthereumLES les, uint64_t reqId, uint64_t* reqLocIndex){
//
//    int ret = 0;
//
//    pthread_mutex_lock(&les->lock);
//    for(int i = 0; i < array_count(les->requests); i++) {
//        if(reqId == les->requests[i].requestId){
//            *reqLocIndex = i;
//            ret = 1;
//            break;
//        }
//    }
//    pthread_mutex_unlock(&les->lock);
//
//    return ret;
//}
//static void _receivedMessageCallback(BREthereumSubProtoContext info, uint64_t messageType, BRRlpData messageBody) {
//
//    BREthereumLES les = (BREthereumLES)info;
//
//    eth_log (ETH_LOG_TOPIC, "Recv [%13s] from peer [%s]",
//             messageLESGetIdentifierName ((BREthereumLESMessageIdentifier) (messageType - les->message_id_offset)),
//             "...");
//
//    switch (messageType - les->message_id_offset)
//    {
//        case BRE_LES_ID_STATUS:
//        {
//            BREthereumLESDecodeStatus remoteStatus = coderDecodeStatus(les->coder, messageBody.bytes, messageBody.bytesCount, &les->peerStatus);
//            if(remoteStatus == BRE_LES_CODER_SUCCESS)
//            {
//                if(ETHEREUM_BOOLEAN_IS_TRUE(les->peerStatus.txRelay) &&
//                   les->peerStatus.chainId == networkGetChainId(les->network)){
//                    eth_log(ETH_LOG_TOPIC, "%s", "LES Handshake complete. Start sending messages");
//                    statusMessageShow (&les->peerStatus);
//                    les->statusFunc (les->announceCtx, les->peerStatus.headHash, les->peerStatus.headNum);
//                    les->startSendingMessages = ETHEREUM_BOOLEAN_TRUE;
//                }else {
//                    eth_log(ETH_LOG_TOPIC, "%s", "Disconnecting from node. Does not meet the requirements for LES");
//                }
//            }
//        }
//            break;
//        case BRE_LES_ID_ANNOUNCE:
//        {
//            BREthereumHash hash;
//            uint64_t headNumber;
//            UInt256 headTd;
//            uint64_t reorgDepth;
//            BREthereumLESDecodeStatus remoteStatus = coderDecodeAnnounce(les->coder, messageBody.bytes, messageBody.bytesCount, &hash, &headNumber, &headTd, &reorgDepth, &les->peerStatus);
//            if(remoteStatus == BRE_LES_CODER_SUCCESS)
//            {
//                les->announceFunc(les->announceCtx, hash, headNumber, headTd, reorgDepth);
//                //TODO: Check to make sure peerStatus is valid after update.
//            }
//        }
//            break;
//        case BRE_LES_ID_TX_STATUS:
//        {
//            //rlpShow(messageBody, "LES-TX_STATUS");
//            uint64_t reqId = 0, bv = 0;
//            BREthereumTransactionStatus* replies;
//            size_t repliesCount;
//            BREthereumLESDecodeStatus status = coderDecodeTxStatus(les->coder, messageBody.bytes, messageBody.bytesCount, &reqId, &bv, &replies, &repliesCount);
//
//            if(status == BRE_LES_CODER_SUCCESS)
//            {
//                uint64_t requestIndexRm = 0;
//
//                if(_findRequestId(les, reqId, &requestIndexRm)){
//                    pthread_mutex_lock(&les->lock);
//                    for(int i = 0; i < repliesCount; ++i){
//                        les->requests[requestIndexRm].u.transaction_status.callback(les->requests[requestIndexRm].u.transaction_status.ctx,
//                                                                                    les->requests[requestIndexRm].u.transaction_status.transactions[i],
//                                                                                    replies[i]);
//                    }
//                    free(les->requests[requestIndexRm].u.transaction_status.transactions);
//                    array_rm(les->requests, requestIndexRm);
//                    free (replies);
//                    pthread_mutex_unlock(&les->lock);
//                }
//            }
//        }
//            break;
//        case BRE_LES_ID_BLOCK_HEADERS:
//        {
//            //            rlpShow(messageBody, "LES-HEADERS");
//            uint64_t reqId = 0, bv = 0;
//            BREthereumBlockHeader* headers;
//            BREthereumLESDecodeStatus status = coderDecodeBlockHeaders(les->coder, messageBody.bytes, messageBody.bytesCount, &reqId, &bv, &headers);
//
//            if(status == BRE_LES_CODER_SUCCESS)
//            {
//                uint64_t requestIndexRm = 0;
//
//                if(_findRequestId(les, reqId, &requestIndexRm)){
//
//                    BREthereumLESBlockHeadersCallback callback = les->requests[requestIndexRm].u.block_headers.callback;
//                    BREthereumLESBlockHeadersContext context = les->requests[requestIndexRm].u.block_headers.context;
//
//                    for(int i = 0; i < array_count(headers); ++i){
//                        callback(context, headers[i]);
//                    }
//
//                    pthread_mutex_lock(&les->lock);
//                    array_rm(les->requests, requestIndexRm);
//                    pthread_mutex_unlock(&les->lock);
//                }
//                array_free (headers);
//            }
//
//        }
//            break;
//        case BRE_LES_ID_BLOCK_BODIES:
//        {
//            // rlpShow(messageBody, "LES-BODIES");
//            uint64_t reqId = 0, bv = 0;
//
//            BREthereumBlockHeader** ommers;
//            BREthereumTransaction** transactions;
//            BREthereumLESDecodeStatus status = coderDecodeBlockBodies(les->coder, messageBody.bytes, messageBody.bytesCount, &reqId, &bv, les->network, &ommers, &transactions);
//
//
//            if(status == BRE_LES_CODER_SUCCESS)
//            {
//                uint64_t requestIndexRm = 0;
//                if(_findRequestId(les, reqId, &requestIndexRm)){
//
//
//                    BREthereumLESBlockBodiesCallback callback = les->requests[requestIndexRm].u.block_bodies.callback;
//                    BREthereumLESBlockBodiesContext context = les->requests[requestIndexRm].u.block_bodies.context;
//                    BREthereumHash* blocks = les->requests[requestIndexRm].u.block_bodies.blocks;
//
//                    for(int i = 0; i < array_count(blocks); ++i){
//                        callback(context, blocks[i],transactions[i],ommers[i]);
//                    }
//
//                    pthread_mutex_lock(&les->lock);
//                    array_free(les->requests[requestIndexRm].u.block_bodies.blocks);
//                    array_rm(les->requests, requestIndexRm);
//                    pthread_mutex_unlock(&les->lock);
//                }
//                array_free (ommers);
//                array_free (transactions);
//            }
//
//        }
//            break;
//        case BRE_LES_ID_RECEIPTS:
//        {
//            // size_t len = 0;
//            // printf("%s", encodeHexCreate(&len, messageBody.bytes, messageBody.bytesCount));
//            // rlpShow(messageBody, "LES-RECEIPTS");
//            uint64_t reqId = 0, bv = 0;
//            BREthereumTransactionReceipt** receipts;
//            BREthereumLESDecodeStatus status = coderDecodeReceipts(les->coder, messageBody.bytes, messageBody.bytesCount, &reqId, &bv, &receipts);
//
//            if(status == BRE_LES_CODER_SUCCESS)
//            {
//                uint64_t requestIndexRm = 0;
//                _findRequestId(les, reqId, &requestIndexRm);
//
//                if(_findRequestId(les, reqId, &requestIndexRm)) {
//                    pthread_mutex_lock(&les->lock);
//
//                    BREthereumLESReceiptsCallback callback = les->requests[requestIndexRm].u.receipts.callback;
//                    BREthereumLESReceiptsContext context = les->requests[requestIndexRm].u.receipts.context;
//                    BREthereumHash* blocks = les->requests[requestIndexRm].u.receipts.blocks;
//
//                    for(int i = 0; i < array_count(blocks); ++i){
//                        callback(context, blocks[i],receipts[i]);
//                    }
//
//                    array_free(les->requests[requestIndexRm].u.receipts.blocks);
//                    array_rm(les->requests, requestIndexRm);
//                    pthread_mutex_unlock(&les->lock);
//                }
//                array_free(receipts);
//            }
//
//        }
//            break;
//        case BRE_LES_ID_PROOFS_V2:
//        {
//            rlpShow(messageBody, "LES-PROOFSV2");
//            uint64_t reqId = 0, bv = 0;
//            BREthereumLESDecodeStatus status = coderDecodeProofs(les->coder, messageBody.bytes, messageBody.bytesCount, &reqId, &bv); // actual return value.
//
//            if(status == BRE_LES_CODER_SUCCESS)
//            {
//                uint64_t requestIndexRm = 0;
//                _findRequestId(les, reqId, &requestIndexRm);
//
//                if(_findRequestId(les, reqId, &requestIndexRm)) {
//                    pthread_mutex_lock(&les->lock);
//
//                    BREthereumLESProofsV2Callback callback = les->requests[requestIndexRm].u.proofsV2.callback;
//                    BREthereumLESProofsV2Context context = les->requests[requestIndexRm].u.proofsV2.context;
//                    BREthereumLESProofsRequest* proofRequests = les->requests[requestIndexRm].u.proofsV2.proofRequests;
//
//                    for(int i = 0; i < array_count(proofRequests); ++i){
//                        // Actual result is TBD
//                        callback (context, proofRequests[i].blockHash, proofRequests[i].key1, proofRequests[i].key2);
//                    }
//
//                    array_free(les->requests[requestIndexRm].u.proofsV2.proofRequests);
//                    array_rm(les->requests, requestIndexRm);
//                    pthread_mutex_unlock(&les->lock);
//                }
//            }
//
//        }
//            break;
//        default:
//        {
//            eth_log(ETH_LOG_TOPIC, "Received Message that cannot be handled, message id:%llu", messageType - les->message_id_offset);
//        }
//            break;
//    }
//}
//
//static void _connectedToNetworkCallback(BREthereumSubProtoContext info, uint8_t** statusBytes, size_t* statusSize){
//    BREthereumLES les = (BREthereumLES)info;
//    BRRlpData statusPayload = coderEncodeStatus(les->coder, les->message_id_offset, &les->statusMsg);
//    *statusBytes = statusPayload.bytes;
//    *statusSize = statusPayload.bytesCount;
//}
//static void _networkReachableCallback(BREthereumSubProtoContext info, BREthereumBoolean isReachable) {}
//
//static BREthereumLESStatus _sendMessage(BREthereumLES les, uint8_t packetType, BRRlpData payload) {
//
//    BREthereumLESNodeManagerStatus status = nodeManagerStatus(les->nodeManager);
//    BREthereumLESStatus retStatus = LES_NETWORK_UNREACHABLE;
//
//    if(ETHEREUM_BOOLEAN_IS_TRUE(les->startSendingMessages)){
//
//        if(status == BRE_MANAGER_CONNECTED)
//        {
//            if(ETHEREUM_BOOLEAN_IS_TRUE(nodeManagerSendMessage(les->nodeManager, packetType, payload.bytes, payload.bytesCount))){
//                retStatus = LES_SUCCESS;
//            }
//        }
//        else
//        {
//            //Retry to connect to the ethereum network
//            if(nodeManagerConnect(les->nodeManager)){
//                sleep(3); //Give it some time to see if the node manager can connect to the network again;
//                if(ETHEREUM_BOOLEAN_IS_TRUE(nodeManagerSendMessage(les->nodeManager, packetType, payload.bytes, payload.bytesCount))){
//                    retStatus = LES_SUCCESS;
//                }
//            }
//        }
//    }
//    return retStatus;
//}
