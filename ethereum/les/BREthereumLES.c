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
#include "BREthereumLESMessage.h"
#include "BREthereumLESNode.h"

/** Forward Declarations */

static void
lesHandleLESMessage (BREthereumLES les,
                     BREthereumLESNode node,
                     BREthereumLESMessage message);
static void
lesHandleNodeState (BREthereumLES les,
                    BREthereumLESNode node,
                    BREthereumLESNodeEndpointRoute route,
                    BREthereumLESNodeState state);

typedef void* (*ThreadRoutine) (void*);

static void *
lesThread (BREthereumLES les);

static inline int maximum (int a, int b) { return a > b ? a : b; }
static inline int minimum (int a, int b) { return a < b ? a : b; }

#define LES_THREAD_NAME    "Core Ethereum LES"
#define LES_PTHREAD_STACK_SIZE (512 * 1024)
#define LES_PTHREAD_NULL   ((pthread_t) NULL)

#define LES_REQUESTS_INITIAL_SIZE   10
#define LES_NODE_INITIAL_SIZE   10

#define DEFAULT_UDPPORT     (30303)
#define DEFAULT_TCPPORT     (30303)

#define LES_LOCAL_ENDPOINT_ADDRESS    "1.1.1.1"
#define LES_LOCAL_ENDPOINT_TCP_PORT   DEFAULT_TCPPORT
#define LES_LOCAL_ENDPOINT_UDP_PORT   DEFAULT_UDPPORT
#define LES_LOCAL_ENDPOINT_NAME       "BRD Light Client"

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
#define FOR_NODES_INDEX( les, index ) \
  for (size_t index = 0; index < array_count ((les)->connectedNodes); index++)

#define FOR_SET(type,var,set) \
  for (type var = BRSetIterate(set, NULL); \
       NULL != var; \
       var = BRSetIterate(set, var))

#define FOR_NODES( les, node )  FOR_SET(BREthereumLESNode, node, les->nodes)

#define FOR_CONNECTED_NODES_INDEX( les, index ) \
    for (size_t index = 0; index < array_count ((les)->connectedNodes); index++)

/// MARK: LES Node Endpoint Spec
const char *localLESEnode = "enode://x@1.1.1.1:30303";

const char *bootstrapLESEnodes[] = {
    // Localhost
//    "enode://a40437d2f44ae655387009d1d69ba9fd07b748b7a6ecfc958c135008a34c0497466db35049c36c8296590b4bcf9b9058f9fa2a688a2c6566654b1f1dc42417e4@127.0.0.1:30303",

    // START - BRD
//    "enode://e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851@104.197.99.24:30303", // full
    "enode://e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851@35.226.238.26:30303", // archival
    // END - BRD

    // START -- https://gist.github.com/rfikki/e2a8c47f4460668557b1e3ec8bae9c11
    "enode://03f178d5d4511937933b50b7af683b467abaef8cfc5f7c2c9b271f61e228578ae192aaafc7f0d8035dfa994e734c2c2f72c229e383706be2f4fa43efbe9f94f4@163.172.149.200:30303",
    "enode://0f740f471e876020566c2ce331c81b4128b9a18f636b1d4757c4eaea7f077f4b15597a743f163280293b0a7e35092064be11c4ec199b9905541852a36be9004b@206.221.178.149:30303",
    "enode://16d92fc94f4ec4386aca44d255853c27cbe97a4274c0df98d2b642b0cc4b2f2330e99b00b46db8a031da1a631c85e2b4742d52f5eaeca46612cd28db41fb1d7f@91.223.175.173:30303",
    "enode://1d70e87a2ee28a2762f1b2cd56f1b9134824a84264030539bba297f67a5bc9ec7ae3016b5f900dc59b1c27b4e258a63fc282a37b2dd6e25a8377473530513394@208.88.169.151:30303",
    "enode://242b68a4e37b4478c46901c3512315f36bd1aa513566d1f061939b202258b55d63d66367bc5807e62ec03ae673bead9a351846e3f23284ce79537ff7afa65615@34.201.26.61:30303",
    "enode://2af1ef12967d112f527648819f89e55bfe61f77f5920a0edc1c21de274092bc4839a68405b13d845a0c133b101050c5fb04f5b4a8683663fc20d9ccc5f68d0f3@34.239.156.26:30303",
    "enode://31b5db1136a0ebceeb0ab6879e95dc66e8c52bcce9c8de50e2f722b5868f782aa0306b6b137b9e0c6271a419c5562a194d7f2abd78e22dcd1f55700dfc30c46a@35.165.17.127:30303",
    "enode://3afdfd40713a8b188a94e4c7a9ddc61bc6ef176c3abbb13d1dd35eb367725b95329a7570039044dbffa49c50d4aa65f0a1f99ee68e46b8e2f09100d11d4fc85a@31.17.196.138:30303",
    "enode://3d0bce4775635c65733b7534f1bccd48720632f5d66a44030c1d13e2e5883262d9d22cdb8365c03137e8d5fbbf5355772acf35b08d6f9b5ad69bb24ad52a20cc@35.184.255.33:30303",
    "enode://4baa9b4ea9f3219e595f52c817ce4829ae916e7b1ea0f356a543c73de0c7d7ff889b6360f4b7dfbbcae7d2f60b51a16bc02ccc510df6be0aee63cba94ff5a923@18.207.138.205:30303",
    "enode://4c2b5c5d9503b7f4e76a551c827f19200f7f9ebb62f2cb5078c352de1e8d4d1006efa8fc143f9ccf2c8fd85836198dc1c69729dfa1c54d63f5d1d57fd8781bf8@62.151.178.212:30303",
    "enode://63acf19ecd1f7a365176cc4ccf0b410e8fa05a60a5b298102a7a0194e86570a6f9e15abbb23cb3791fd92ddd4e25d32dba7a6c6887f6b76e4b266288fa99cf98@76.170.48.252:30303",
    "enode://89495deb21261a4542d50167d6e69cf3b1a585609e6843a23becbd349d92755bd2ddcc55bb1f2c017099b774454d95ef5ebccbed1859fc530fb34843ddfc32e2@52.39.91.131:30303",
    "enode://95176fe178be55d40aae49a5c11f21aa58968e13c681a6b1f571b2bb3e45927a7fb3888361bef85c0e28a52ea0e4afa17dcaa9d6c61baf504b3559f056f78581@163.172.145.241:30303",
    "enode://a979fb575495b8d6db44f750317d0f4622bf4c2aa3365d6af7c284339968eef29b69ad0dce72a4d8db5ebb4968de0e3bec910127f134779fbcb0cb6d3331163c@52.16.188.185:30303",
    "enode://ae1d9252428fa66371bc68e9c4fc0f9c60d09943b521cede6c60b50c67fd6dc1d614525c07030afe52586cbf35d43ad83368ad71c57639125698c3392f8b4a89@121.140.198.219:30303",
    "enode://bfad505cbb2bde72e161a7cff044d66d20ceb85c8a61047b50037881f289bd2dcc064189ade2077daddd5b20fd2fc6dee7208f227ae2a34361bf51751d225e8e@51.15.220.91:30303",
    "enode://d324187ba8da3ac7ad454eeb9aa395eae610fc032bccf9dae63c1e3206458cf55c7e9e454ce23acf9706fb89d0ce9d47038ab261676776b5c6fa1b76c6cf829c@198.58.126.224:30303",
    "enode://d5d63b7b26027d54f1d03656d8aed536b3c914999cbedddf7a4733e1286984ae99ebe2e7a1b3ada1ae4b10af4ddd9c5ed235ef908795f7142ef2061ca1751a11@198.74.52.106:30303",
    "enode://d70756f1aa07246a61731c8b0ce3e89046e07e8a18c497172dd4baa4b380998b4ee669396140effe65affbcd79bb269ec3f2c698b97507656291c88e7f8e1bc3@50.116.21.236:30303",
//    "enode://e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851@104.197.99.24:30303",
    "enode://ea1737bf696928b4b686a2ccf61a6f2295d149281a80b0d83a9bce242e7bb084434c0837a2002d4cc2840663571ecf3e45517545499c466e4373c69951d090fe@163.172.181.92:30303",
    "enode://f251404ab66f10df6f541d69d735616a7d78e04673ec40cdfe6bf3d1fb5d84647ba627f22a1e8c5e2aa45629c88e33bc394cc1633a63fed11d84304892e51fe9@196.54.41.2:38065",
    // END -- https://gist.github.com/rfikki/e2a8c47f4460668557b1e3ec8bae9c11

    // Random
    "enode://3e9301c797f3863d7d0f29eec9a416f13956bd3a14eec7e0cf5eb56942841526269209edf6f57cd1315bef60c4ebbe3476bc5457bed4e479cac844c8c9e375d3@109.232.77.21:30303", // GETH
    "enode://81863f47e9bd652585d3f78b4b2ee07b93dad603fd9bc3c293e1244250725998adc88da0cef48f1de89b15ab92b15db8f43dc2b6fb8fbd86a6f217a1dd886701@193.70.55.37:30303",  // Parity
};
#define PRIMARY_BOOTSTRAP_NODE_INDEX 2

#define NUMBER_OF_NODE_ENDPOINT_SPECS   (sizeof (bootstrapLESEnodes) / sizeof (char *))

static BREthereumLESNodeEndpoint
nodeEndpointCreateLocal (BREthereumLESRandomContext randomContext) {
    BREthereumDISEndpoint dis = {
        AF_INET,
        {},
        LES_LOCAL_ENDPOINT_UDP_PORT,
        LES_LOCAL_ENDPOINT_TCP_PORT
    };

    inet_pton (dis.domain, LES_LOCAL_ENDPOINT_ADDRESS, &dis.addr);

    BRKey localKey, localEphemeralKey;
    UInt256 localNonce;

    randomGenPriKey  (randomContext, &localKey);
    randomGenPriKey  (randomContext, &localEphemeralKey);
    randomGenUInt256 (randomContext, &localNonce);

    assert (0 == localKey.compressed);

    return nodeEndpointCreateDetailed (dis, localKey, localEphemeralKey, localNonce);
}

static BREthereumLESNodeEndpoint
nodeEndpointCreateEnode (const char *enode) {
    size_t enodeLen = strlen (enode);
    assert (enodeLen < 1024);

    char buffer[1024], *buf = buffer;
    assert (1 == sscanf (enode, "enode://%s", buffer));

    char *id = strsep (&buf, "@:");
    char *ip = strsep (&buf, "@:");
    char *pt = strsep (&buf, "@:");
    int port = atoi (pt);

    BREthereumDISEndpoint dis = {
        AF_INET,
        {},
        port,
        port
    };

    inet_pton (dis.domain, ip, &dis.addr);

    BRKey key;
    key.pubKey[0] = 0x04;
    key.compressed = 0;
    decodeHex(&key.pubKey[1], 64, id, 128);
    return nodeEndpointCreate(dis, key);
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
    uint64_t requestId;
    BREthereumLESMessage message;

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

    /**
     * The node that sent this request. This will be NULL until that request has been successfully
     * sent.  Once sent, if this request has not be received/resolved and the node is no longer
     * connected, then we'll resend.
     */
     BREthereumLESNode node;

    /**
     * The node index to use when sending.  Most will use the PREFFERED_NODE_INDEX of 0 - which is
     * the default node.  But, for a SendTx message, we'll send to multiple nodes,
     * if connected.
     */
    size_t nodeIndex;

} BREthereumLESReqeust;

/**
 * Create a LES Request
 */
static BREthereumLESReqeust
lesRequestCreate (BREthereumLES les,
                  uint64_t requestId,
                  void *context,
                  void (*callback) (),
                  BREthereumLESMessage message,
                  size_t nodeIndex) {

    switch (message.identifier) {
        case LES_MESSAGE_GET_BLOCK_HEADERS:
            return (BREthereumLESReqeust) {
                requestId,
                message,
                { .getBlockHeaders = {
                    (BREthereumLESBlockHeadersCallback) callback,
                    (BREthereumLESBlockHeadersContext) context }},
                NULL,
                nodeIndex
            };
            break;

        case LES_MESSAGE_GET_BLOCK_BODIES:
            return (BREthereumLESReqeust) {
                requestId,
                message,
                { .getBlockBodies = {
                    (BREthereumLESBlockBodiesCallback) callback,
                    (BREthereumLESBlockBodiesContext) context,
                    message.u.getBlockBodies.hashes }},
                NULL,
                nodeIndex
            };
            break;

        case LES_MESSAGE_GET_RECEIPTS:
            return (BREthereumLESReqeust) {
                requestId,
                message,
                { .getReceipts = {
                    (BREthereumLESReceiptsCallback) callback,
                    (BREthereumLESReceiptsContext) context,
                    message.u.getReceipts.hashes }},
                NULL,
                nodeIndex
            };
            break;

        case LES_MESSAGE_GET_TX_STATUS:
            return (BREthereumLESReqeust) {
                requestId,
                message,
                { .getTxStatus = {
                    (BREthereumLESTransactionStatusCallback) callback,
                    (BREthereumLESTransactionStatusContext) context,
                    message.u.getTxStatus.hashes }},
                NULL,
                nodeIndex
            };
            break;

        case LES_MESSAGE_SEND_TX2: {
            // A LESv2 message of SendTx2 is sent; the response is a txStatus
            size_t transactionsCount = array_count (message.u.sendTx2.transactions);
            BREthereumHash hashes [transactionsCount];
            for (size_t index = 0; index < transactionsCount; index++)
                hashes[index] = transactionGetHash(message.u.sendTx2.transactions[index]);

            return (BREthereumLESReqeust) {
                requestId,
                message,
                { .getTxStatus = {
                    (BREthereumLESTransactionStatusCallback) callback,
                    (BREthereumLESTransactionStatusContext) context,
                    hashes }},
                NULL,
                nodeIndex
            };
            break;
        }
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
            return (BREthereumLESReqeust) {
                requestId,
                message,
                { .getProofsV2 = {
                    (BREthereumLESProofsV2Callback) callback,
                    (BREthereumLESProofsV2Context) context,
                    message.u.getProofs.specs }},
                NULL,
                nodeIndex
            };
            break;

        default:
            assert (0);
            break;
    }
}

/**
 * LES
 *
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

    /** Our Local Endpoint. */
    BREthereumLESNodeEndpoint localEndpoint;

    /** Array of Nodes */
    BRSetOf(BREthereumLESNode) nodes;
    BRArrayOf(BREthereumLESNode) connectedNodes;
//    BREthereumLESNode preferredDISNode;
//    BREthereumLESNode preferredLESNode;

    uint64_t messageRequestId;

    BRArrayOf (BREthereumLESReqeust) requests;
    BRArrayOf (BREthereumLESReqeust) requestsToSend;

    /** Thread */
    pthread_t thread;
    pthread_mutex_t lock;

    /** replace with pipe() message */
    int theTimeToQuitIsNow;
};

static void
assignLocalEndpointHelloMessage (BREthereumLESNodeEndpoint *endpoint,
                                 BREthereumLESNodeType type) {
    // From https://github.com/ethereum/wiki/wiki/ÐΞVp2p-Wire-Protocol on 2019 Aug 21
    // o p2pVersion: Specifies the implemented version of the P2P protocol. Now must be 1
    // o listenPort: specifies the port that the client is listening on (on the interface that the
    //    present connection traverses). If 0 it indicates the client is not listening.
    BREthereumP2PMessage hello = {
        P2P_MESSAGE_HELLO,
        { .hello  = {
            0x01,
            strdup (LES_LOCAL_ENDPOINT_NAME),
            NULL, // capabilities
            endpoint->dis.portTCP,
            {}
        }}};

    array_new (hello.u.hello.capabilities, 1);
    switch (type) {
        case NODE_TYPE_GETH:
            array_add (hello.u.hello.capabilities,
                       ((BREthereumP2PCapability) { "les", 2 }));
            break;

        case NODE_TYPE_PARITY:
            array_add (hello.u.hello.capabilities,
                       ((BREthereumP2PCapability) { "pip", 1 }));
            break;
    }

    // The NodeID is the 64-byte (uncompressed) public key
    uint8_t pubKey[65];
    assert (65 == BRKeyPubKey (&endpoint->key, pubKey, 65));
    memcpy (hello.u.hello.nodeId.u8, &pubKey[1], 64);

    nodeEndpointSetHello (endpoint, hello);
    messageP2PHelloShow (hello.u.hello);
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
    
    BREthereumLES les = (BREthereumLES) calloc (1, sizeof(struct BREthereumLESRecord));
    assert (NULL != les);

    // For now, create a new, random private key that is used for communication with LES nodes.
    UInt256 secret;
#if defined (__ANDROID__)
    assert (false);
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

    {
        // Use the privateKey to create a randomContext
        BREthereumLESRandomContext randomContext =  randomCreate (les->key.secret.u8, 32);

        // Create a local endpoint; when creating nodes we'll use this local endpoint repeatedly.
        les->localEndpoint = nodeEndpointCreateLocal(randomContext);

        randomRelease (randomContext);
    }

    // The 'hello' message is fixed; assign it to the local endpoint. Truth be that the
    // hello message depends on the node type, GETH or PARITY.  If we ever actually support
    // both we either: need two local endpoints (one for GETH; one for PARITY); or perhaps we
    // can specify the local capabilities as [ { "les", 2 }, { "pip", 1 } ] - but then require
    // the remote to have one of the two, instead of all, like now.
    assignLocalEndpointHelloMessage (&les->localEndpoint, NODE_TYPE_GETH);

    // The 'status' message MIGHT BE fixed; create one and assign it to the local endoint.  Like
    // the 'hello' message, the status message likely depends on teh node type, GETH or PARITY.
    // Specifically the `protocolVersion` is 2 for GETH (our expectation) and 1 for PARITY and the
    // `announceType` is only specified for a GETH protocol of 2.
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

    les->messageRequestId = 0;

    // TODO: message Request (Set)
    array_new (les->requests, LES_REQUESTS_INITIAL_SIZE);
    array_new (les->requestsToSend, LES_REQUESTS_INITIAL_SIZE);

    // The Set of all known nodes.
    les->nodes = BRSetNew (nodeHashValue,
                           nodeHashEqual,
                           10 * LES_NODE_INITIAL_SIZE);

    // (Prioritized) array of connected Nodes (both TCP and UDP connections)
    array_new (les->connectedNodes, LES_NODE_INITIAL_SIZE);
    
    // Create a node for each bootstrap node
    for (size_t index = 0; index < NUMBER_OF_NODE_ENDPOINT_SPECS; index++) {
        BREthereumLESNode node = nodeCreate (network,
                                             nodeEndpointCreateEnode(bootstrapLESEnodes[index]),
                                             les->localEndpoint,
                                             (BREthereumLESNodeContext) les,
                                             (BREthereumLESNodeCallbackMessage) lesHandleLESMessage,
                                             (BREthereumLESNodeCallbackState) lesHandleNodeState);
        // Must not be be a duplicate nodeID
        assert (NULL == BRSetAdd (les->nodes, node));
    }
    
//    les->preferredDISNode = node;
//    les->preferredLESNode = node;

    les->theTimeToQuitIsNow = 0;
    return les;
}

extern void
lesStart (BREthereumLES les) {
//    FOR_NODES_INDEX(les, index)
//        nodeConnect (les->nodes[index]);

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

extern void lesRelease(BREthereumLES les) {
    lesStop (les);
    pthread_mutex_lock (&les->lock);

    FOR_NODES (les, node)
        nodeRelease (node);

    BRSetClear(les->nodes);
    array_free (les->connectedNodes);

    rlpCoderRelease(les->coder);

    // requests, requestsToSend

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

/// MARK: Node Management

/**
 * Create a LES node from a DIS neighbor.  The result of a DIS 'Find Neighbors' request will be
 * a list of BREthereumDISNeighbor; we'll create and add them.
 *
 * @param les
 * @param neighbor
 * @return
 */
static BREthereumLESNode
lesNodeCreate (BREthereumLES les,
               BREthereumDISNeighbor neighbor) {
    BRKey key;
    key.compressed = 0;
    key.pubKey[0] = 0x04;
    memcpy (&key.pubKey[1], neighbor.nodeID.u8, sizeof (neighbor.nodeID));

    return nodeCreate (les->network,
                       nodeEndpointCreate(neighbor.node, key),
                       les->localEndpoint,
                       (BREthereumLESNodeContext) les,
                       (BREthereumLESNodeCallbackMessage) lesHandleLESMessage,
                       (BREthereumLESNodeCallbackState) lesHandleNodeState);
}

/**
 * Check if there is already a node for neighbor.  If there is, do nothing; if there isn't then
 * create a node and add it.
 */
static BREthereumBoolean
lesHandleNeighbor (BREthereumLES les,
                   BREthereumDISNeighbor neighbor) {
    BREthereumBoolean added = ETHEREUM_BOOLEAN_FALSE;
    pthread_mutex_lock (&les->lock);
    if (NULL == BRSetGet(les->nodes, &neighbor.nodeID)) {
        BREthereumLESNode node = lesNodeCreate(les, neighbor);
        BRSetAdd (les->nodes, node);
        added = ETHEREUM_BOOLEAN_TRUE;
        // We are not interested in 'strange-ish' nodes

        if (DEFAULT_TCPPORT != neighbor.node.portTCP)
            nodeSetStateErrorProtocol (node, NODE_ROUTE_TCP);

        if (DEFAULT_UDPPORT != neighbor.node.portUDP)
            nodeSetStateErrorProtocol (node, NODE_ROUTE_UDP);
    }
    pthread_mutex_unlock (&les->lock);
    return added;
}

static void
lesNodeAddConnected (BREthereumLES les,
                     BREthereumLESNode node) {
    int needLog = 0;
    size_t count = 0;

    pthread_mutex_lock (&les->lock);
    FOR_CONNECTED_NODES_INDEX(les, index)
        // If we already have node...
        if (node == les->connectedNodes[index]) {
            node = NULL;
            break;
        }
    // .. avoid adding it again.
    if (NULL != node) {
        array_add (les->connectedNodes, node);
        count = array_count (les->connectedNodes);
        needLog = 1;
    }
    pthread_mutex_unlock (&les->lock);

    if (needLog)
        eth_log (LES_LOG_TOPIC, "Connect: (%zu): <===> %s",
                 count,
                 nodeGetRemoteEndpoint(node)->hostname);
}

static void
lesNodeRemConnected (BREthereumLES les,
                     BREthereumLESNode node) {
    int needLog = 0;
    size_t count = 0;
    pthread_mutex_lock (&les->lock);
    FOR_CONNECTED_NODES_INDEX(les, index)
        if (node == les->connectedNodes[index]) {
            array_rm (les->connectedNodes, index);
            count = array_count(les->connectedNodes);
            needLog = 1;
            break;
        }
    pthread_mutex_unlock (&les->lock);
    if (needLog)
        eth_log (LES_LOG_TOPIC, "Connect: (%zu): <=|=> %s",
                 count,
                 nodeGetRemoteEndpoint(node)->hostname);

}

static size_t
lesNodeAvailableCount (BREthereumLES les) {
    size_t count = 0;
    pthread_mutex_lock (&les->lock);
    FOR_NODES (les, node)
        count += (nodeHasState(node, NODE_ROUTE_UDP, NODE_AVAILABLE) &&
                  nodeHasState(node, NODE_ROUTE_TCP, NODE_AVAILABLE));
    pthread_mutex_unlock (&les->lock);
    return count;
}

static void
lesNodeFindNeighborsToEndpoint (BREthereumLES les,
                                BREthereumLESNode node,
                                BREthereumLESNodeEndpoint *endpoint) {
    BREthereumMessage findNodes = {
        MESSAGE_DIS,
        { .dis = {
            DIS_MESSAGE_FIND_NEIGHBORS,
            { .findNeighbors =
                messageDISFindNeighborsCreate (endpoint->key,
                                               time(NULL) + 1000000) },
            nodeGetLocalEndpoint(node)->key }}
    };
    nodeSend (node, NODE_ROUTE_UDP, findNodes);
    eth_log (LES_LOG_TOPIC, "Neighbors: %15s", endpoint->hostname);
}

//static BREthereumLESNode
//lesGetPreferredDISNode (BREthereumLES les) {
//    return les->nodes[0];
//}
//
//static BREthereumLESNode
//lesGetPreferredP2PNode (BREthereumLES les) {
//    return les->nodes[0];
//}
//
//static int
//lesIsDISNodeNeeded (BREthereumLES les) {
//    return 0;
//}
//
//static int
//lesIsP2PNodeNeeded (BREthereumLES les) {
//    return 1;
//}
//
//static int
//lesIsDISPortOfInterest (BREthereumLES les, int port) {
//    return port == DEFAULT_UDPPORT;
//}
//
//static int
//lesIsP2PPortOfInterest (BREthereumLES les, int port) {
//    return port == DEFAULT_TCPPORT;
//}

#if 0
//static void lesConnectNodeIfAppropriate (BREthereumLES les,
//                                         BREthereumLESNodeEndpointRoute route) {
//    unsigned int counts[NUMBER_OF_NODE_ROUTES];
//
//    for (int route = 0; route < NUMBER_OF_NODE_ROUTES; route++) {
//        counts[route] = 0;
//        FOR_NODES_INDEX (les, index)
//            counts[route] += nodeHasState (les->nodes[index],
//                                           (BREthereumLESNodeEndpointRoute) route,
//                                           NODE_CONNECTED);
//    }
//
//    if (counts[route] < 5) {
//        FOR_NODES_INDEX (les, index) {
//            if (counts[route] < 5 && nodeHasState(les->nodes[index], route, NODE_AVAILABLE)) {
//                nodeConnect (les->nodes[index], route);
//                counts[route] += 1;
//            }
//        }
//    }
//}
//
//static BREthereumLESNode
//lesConnectNodeFindAny (BREthereumLES les,
//                       BREthereumLESNodeEndpointRoute route) {
//    FOR_NODES_INDEX (les, index)
//        if (nodeHasState(les->nodes[index], route, NODE_CONNECTED))
//            return les->nodes[index];
//    return NULL;
//}
#endif

/// MARK: Request Management

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

static void
lesAddReqeust (BREthereumLES les,
               uint64_t requestId,
               void *context,
               void (*callback) (),
               BREthereumLESMessage message,
               size_t nodeIndex) {
    pthread_mutex_lock (&les->lock);
    array_add (les->requestsToSend,
               lesRequestCreate (les, requestId, context, callback, message, nodeIndex));
    pthread_mutex_unlock (&les->lock);
}


static void
lesSendAllRequests (BREthereumLES les) {
    pthread_mutex_lock (&les->lock);
    // Handle all requestsToSend, if we can.
    while (array_count (les->requestsToSend) > 0) {
        size_t connectedNodesCount = array_count (les->connectedNodes);

        // If we've no connected nodes, then there is no point continuing.  (I don't think this
        // value can change while we've got the les->lock.  Anyways, 'belt-and-suspenders')
        if (0 == connectedNodesCount) break;

        // Get the next LES request.
        BREthereumLESReqeust request = les->requestsToSend[0];

        // Get the request's desired nodeIndex.
        size_t index = request.nodeIndex;

        // If we are willing to wrap the index (by number of connected nodes), then do so.
        // Example: connectedNodesCount = 2, index = 3 => index = 1.  Note: index = 3 implies
        // we have four requests {0, 1, 2, 3 }; with connectedNodesCount = 2 we'll send to
        // {0, 1, 0, 1 }.
#if LES_WRAP_SEND_WHEN_MULTIPLE_INDEX
        index %= connectedNodesCount;
#endif

        // If the desired index (via nodeIndex) is too large, then drop the request and continue.
        // There is danger here - but note that we should have gotten at least on send off.
        if (index >= connectedNodesCount) {
            array_rm (les->requestsToSend, 0);
            continue; // don't break; not an error.
        }

        // Cache the node - again 'belt-and-suspenders'
        BREthereumLESNode node = les->connectedNodes[index];

        // And send the request's LES message
        BREthereumLESNodeStatus status = nodeSend (node,
                                                   NODE_ROUTE_TCP,
                                                   (BREthereumMessage) {
                                                       MESSAGE_LES,
                                                       { .les = request.message }});

        // If the send failed, then skip out to fight another day.
        if (NODE_STATUS_ERROR == status) break;

        // Mark as 'sent' (request is a value type - don't forget (for upcoming array_add)).
        request.node = node;

        // Transfer from `reqeustsToSend` over to `requests(Pending)`.
        array_rm (les->requestsToSend, 0);
        array_add (les->requests, request);
    }
    pthread_mutex_unlock (&les->lock);
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

/// MARK: Handle Messages

//static void
//lesSendMessage (BREthereumLES les,
//                BREthereumMessage message) {
//    // TODO: Connected?
//    pthread_mutex_lock(&les->lock);
//    switch (message.identifier) {
//        case MESSAGE_DIS:
//            nodeSend(les->preferredDISNode, NODE_ROUTE_UDP, message);
//            break;
//
//        default:
//            nodeSend(les->preferredLESNode, NODE_ROUTE_TCP, message);
//
//            // If sendTx, send to multiple nodes
//            break;
//
//    }
//    pthread_mutex_unlock(&les->lock);
//}

static void
lesHandleLESMessage (BREthereumLES les,
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
            switch (request.message.identifier) {
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

                case LES_MESSAGE_GET_PROOFS_V2: {
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

                    if (LES_MESSAGE_PROOFS_V2 == message.identifier) {
                        // TODO: Failed because we get proofs of []
                        // assert (array_count(request.u.getProofsV2.specs) == array_count(message.u.proofs.paths));
                        // for (size_t index = 0; index < array_count (message.u.proofs.paths); index++) {
                        for (size_t index = 0; index < array_count (request.u.getProofsV2.specs); index++) {
                            BREthereumLESMessageGetProofsSpec spec = request.u.getProofsV2.specs[index];

                            // We'll actually ignore the proofs result.
                            BREthereumLESAccountStateContext  context  = (BREthereumLESAccountStateContext ) request.u.getProofsV2.context;
                            BREthereumLESAccountStateCallback callback = (BREthereumLESAccountStateCallback) request.u.getProofsV2.callback;

                            BREthereumLESAccountStateResult result = { ACCOUNT_STATE_ERROR_X };

                            for (int i = 0; UINT64_MAX != map[i].number; i++)
                                if (spec.blockNumber < map[i].number) {
                                    result.status = ACCOUNT_STATE_SUCCCESS;
                                    result.u.success.block = spec.blockHash;
                                    result.u.success.address =  spec.address;
                                    result.u.success.accountState = map [i - 1].state;
                                    break;
                                }

                            callback (context, result);
                        }
                    }
                    break;
                }

                default:
                    break;
            }

            array_rm (les->requests, requestIndex);
        }
            break;
    }
}

static void
lesHandleDISMessage (BREthereumLES les,
                     BREthereumLESNode node,
                     BREthereumDISMessage message) {
    switch (message.identifier) {
        case DIS_MESSAGE_PING: {
            // Immediately send a pong message
            BREthereumMessage pong = {
                MESSAGE_DIS,
                { .dis = {
                    DIS_MESSAGE_PONG,
                    { .pong =
                        messageDISPongCreate (message.u.ping.to,
                                              message.u.ping.hash,
                                              time(NULL) + 1000000) },
                    nodeGetLocalEndpoint(node)->key }}
            };
            nodeSend (node, NODE_ROUTE_UDP, pong);
            break;
        }

        case DIS_MESSAGE_NEIGHBORS: {
            // Create and add a node for each neighbor.
            unsigned int count = 0;
            for (size_t index = 0; index < array_count (message.u.neighbors.neighbors); index++) {
                BREthereumDISNeighbor neighbor = message.u.neighbors.neighbors[index];
                if (lesHandleNeighbor (les, neighbor) == ETHEREUM_BOOLEAN_TRUE)
                    count++;
            }

            if (count > 0)
                eth_log (LES_LOG_TOPIC, "Neighbors: Added: %d", count);
            break;
        }
        case DIS_MESSAGE_PONG:
            break;

        case DIS_MESSAGE_FIND_NEIGHBORS:
            break;
    }
}

static void
lesHandleP2PMessage (BREthereumLES les,
                     BREthereumLESNode node,
                     BREthereumP2PMessage message) {
    switch (message.identifier) {
        case P2P_MESSAGE_DISCONNECT:
            nodeDisconnect(node, NODE_ROUTE_TCP, message.u.disconnect.reason);
            break;

        case P2P_MESSAGE_PING: {
            BREthereumMessage pong = {
                MESSAGE_P2P,
                { .p2p = {
                    P2P_MESSAGE_PONG,
                    {}}}
            };
            nodeSend (node, NODE_ROUTE_TCP, pong);
        }

        case P2P_MESSAGE_PONG:
            break;

        case P2P_MESSAGE_HELLO:
            break;
    }
}

/**
 * Handle Node callbacks on state changes.  We won't actually track individual changes, generally;
 * we are only interested in 'fully' connected or not.
 */
static void
lesHandleNodeState (BREthereumLES les,
                    BREthereumLESNode node,
                    BREthereumLESNodeEndpointRoute route,
                    BREthereumLESNodeState state) {

    // If the callback reports an error; then disconnect that route.  But, if the error is on the
    // TCP route, also disconnect the UDP route.
    //
    // TODO: Can't do this.  This callback is called within the node thread;
    // Effectively, can't self-kill.
    // If we are in an error state, won't the thread just exit with the error state defined?  Or
    // are their other callbacks to here that *didn't* set the state.
    //
    // Thank you, Xcode 'thread sanitizer'
    //
    //    if (nodeHasErrorState (node, route)) {
    //        nodeDisconnect (node, route, /* ignore next */ P2P_MESSAGE_DISCONNECT_REQUESTED);
    //        if (NODE_ROUTE_TCP == route)
    //            nodeDisconnect (node, NODE_ROUTE_UDP, /* ignore next */ P2P_MESSAGE_DISCONNECT_REQUESTED);
    //    }

    // If `node` is 'fully' connected (both ROUTE_UDP ad ROUTE_TCP) make it active...
    if (nodeHasState (node, NODE_ROUTE_UDP, NODE_CONNECTED) &&
        nodeHasState (node, NODE_ROUTE_TCP, NODE_CONNECTED))
        lesNodeAddConnected (les, node);

    // ... otherwise, ensure `node` is inactive.
    else lesNodeRemConnected (les, node);
}

static void
lesHandleNodeRead (BREthereumLES les,
                   BREthereumLESNodeEndpointRoute route,
                   BREthereumLESNode node) {
    BREthereumLESNodeMessageResult result = nodeRecv (node, route);
    if (NODE_STATUS_ERROR == result.status) {
        return;
    }
    BREthereumMessage message = result.u.success.message;

    switch (message.identifier) {
        case MESSAGE_P2P:
            lesHandleP2PMessage (les, node, message.u.p2p);
            break;

        case MESSAGE_DIS:
            lesHandleDISMessage (les, node, message.u.dis);
            break;

        case MESSAGE_ETH:
            break;

        case MESSAGE_LES:
            lesHandleLESMessage (les, node, message.u.les);
            break;
    }
}

#if 0
static void
lesHandleTimeout (BREthereumLES les) {
    // If we don't have enough connected nodes, connect some

    lesConnectNodeIfAppropriate(les, NODE_ROUTE_UDP);
    lesConnectNodeIfAppropriate(les, NODE_ROUTE_TCP);

//    if (!nodeHasState(les->preferredDISNode, NODE_ROUTE_UDP, NODE_CONNECTED))
//        nodeConnect(les->preferredDISNode, NODE_ROUTE_UDP);
//
//    if (!nodeHasState(les->preferredLESNode, NODE_ROUTE_TCP, NODE_CONNECTED))
//        nodeConnect(les->preferredLESNode, NODE_ROUTE_TCP);
    BREthereumLESNode node;

    node = lesConnectNodeFindAny(les, NODE_ROUTE_UDP);
    if (NULL != node) les->preferredDISNode = node;

    node = lesConnectNodeFindAny(les, NODE_ROUTE_TCP);
    if (NULL != node) les->preferredLESNode = node;

    // If we don't have enough nodes, discover some
    node = les->preferredDISNode;
    if (array_count(les->nodes) < 10 && nodeHasState(node, NODE_ROUTE_UDP, NODE_CONNECTED)) {
        BREthereumMessage findNodes = {
            MESSAGE_DIS,
            { .dis = {
                DIS_MESSAGE_FIND_NEIGHBORS,
                { .findNeighbors =
                    messageDISFindNeighborsCreate (nodeGetRemoteEndpoint(node)->key,
                                                   time(NULL) + 1000000) },
                nodeGetLocalEndpoint(node)->key }}
        };
        nodeSend (node, NODE_ROUTE_UDP, findNodes);
    }
}
#endif

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
    pthread_setname_np (node->thread, LES_THREAD_NAME);
#else
    pthread_setname_np (LES_THREAD_NAME);
#endif
    pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);
    // pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);

    // TODO: Don't timeout pselect(); get some 'wakeup descriptor'
    struct timespec timeout = { 0, 250000000 }; // .250 seconds

    //
    fd_set readDescriptors;
    int maximumDescriptor = -1;

    // True is we need to update descriptors.
    int updateDesciptors = 1;

    pthread_mutex_lock (&les->lock);

    while (!les->theTimeToQuitIsNow) {
        // Send any/all pending requests.  Might be better to wait on a 'can write'?
        lesSendAllRequests (les);

        // Update the read (and write) descriptors to include nodes that are connected.
        if (updateDesciptors) {
            maximumDescriptor = -1;
            FD_ZERO (&readDescriptors);
            FOR_CONNECTED_NODES_INDEX(les, index)
                maximumDescriptor = maximum (maximumDescriptor,
                                             nodeUpdateDescriptors(les->connectedNodes[index], &readDescriptors, NULL));
            updateDesciptors = 0;
        }

        pthread_mutex_unlock (&les->lock);
        int selectCount = pselect (1 + maximumDescriptor, &readDescriptors, NULL, NULL, &timeout, NULL);
        pthread_mutex_lock (&les->lock);
        if (les->theTimeToQuitIsNow) continue;

        // We have a node ready to process ...
        if (selectCount > 0) {
            FOR_CONNECTED_NODES_INDEX(les, index) {
                BREthereumLESNode node = les->connectedNodes[index];
                int nodeLostConnection = 0;

                if (nodeCanProcess (node, NODE_ROUTE_UDP, &readDescriptors)) {
                    lesHandleNodeRead(les, NODE_ROUTE_UDP, node);
                    nodeLostConnection |= !nodeHasState(node, NODE_ROUTE_UDP, NODE_CONNECTED);
                }

                if (nodeCanProcess (node, NODE_ROUTE_TCP, &readDescriptors)) {
                    lesHandleNodeRead(les, NODE_ROUTE_TCP, node);
                    nodeLostConnection |= !nodeHasState(node, NODE_ROUTE_TCP, NODE_CONNECTED);
                }

                if (nodeLostConnection) {
                    lesNodeRemConnected (les, node);
                    updateDesciptors = 1;
                }
            }
        }

        // or we have a timeout ... nothing to receive; nothing to send
        else if (selectCount == 0) {
            // If a pending request was sent to a node that is no longer connected, then there
            // is no response coming (we are here because 'nothing to receive').  Resend.
            //
            // We are going to iterate back-to-front across les->requests and add each 'orphaned'
            // request to the front of les->requestsToSend - thereby keeping the order quasi-
            // consistent (not that it matters, until proven otherwise).
            for (size_t index = array_count(les->requests); index > 0; index--) {
                BREthereumLESNode node = les->requests[index - 1].node;
                assert (NULL != node);
                if (!nodeHasState (node, NODE_ROUTE_TCP, NODE_CONNECTED)) {
                    // `request` is a value type - don't forget....
                    BREthereumLESReqeust request = les->requests[index - 1];
                    array_rm (les->requests, (index - 1)); // safe, when going back-to-front
                    request.node = NULL;
                    array_insert (les->requestsToSend, 0, request); // to the front
                    eth_log (LES_LOG_TOPIC, "Rtry: [ LES, %15s ]",
                             messageLESGetIdentifierName (request.message.identifier));
                }
            }

            // If we don't have enough connectedNodes, try to add one
            if (array_count(les->connectedNodes) < 5) {
                FOR_NODES (les, node)
                    if (nodeHasState (node, NODE_ROUTE_UDP, NODE_AVAILABLE) &&
                        nodeHasState (node, NODE_ROUTE_TCP, NODE_AVAILABLE)) {
                            nodeConnect (node, NODE_ROUTE_UDP);
                            nodeConnect (node, NODE_ROUTE_TCP);
                            break; // FOR NODES
                        }
            }

            // If we don't have enough availableNodes, try to find some
            if (lesNodeAvailableCount(les) < 25 && array_count(les->connectedNodes) > 0) {
                // We'll ask one of our connected nodes about neighbors to other nodes
                unsigned int remainingToAsk = 3;
                FOR_NODES (les, otherNode)
                    if (ETHEREUM_BOOLEAN_IS_FALSE (nodeGetDiscovered (otherNode))) {
                        if (remainingToAsk-- == 0) break; // FOR_NODES
                        lesNodeFindNeighborsToEndpoint (les, les->connectedNodes[0], nodeGetRemoteEndpoint(otherNode));
                        nodeSetDiscovered (otherNode, ETHEREUM_BOOLEAN_TRUE);
                    }
            }

            updateDesciptors = 1;

            // pipe ()
        }

        // or we have an error.
        else {
            lesHandleSelectError (les, errno);
            updateDesciptors = 1;
        }
    }

    pthread_mutex_unlock (&les->lock);

    eth_log (LES_LOG_TOPIC, "Stop: Nodes: %zu, Available: %zu, Connected: %zu",
             BRSetCount(les->nodes),
             lesNodeAvailableCount (les),
             array_count (les->connectedNodes));

    FOR_NODES (les, node) {
        nodeShow (node);
        nodeDisconnect (node, NODE_ROUTE_UDP, P2P_MESSAGE_DISCONNECT_REQUESTED);
        nodeDisconnect (node, NODE_ROUTE_TCP, P2P_MESSAGE_DISCONNECT_REQUESTED);
    }

    pthread_exit (0);
}

/// ==============================================================================================
///
/// (Primary) Public Interface -
///

/// MARK: Get Block Headers

extern void
lesGetBlockHeaders (BREthereumLES les,
                    BREthereumLESBlockHeadersContext context,
                    BREthereumLESBlockHeadersCallback callback,
                    uint64_t blockNumber,
                    uint32_t maxBlockCount,
                    uint64_t skip,
                    BREthereumBoolean reverse) {
    uint64_t requestId = lesGetThenIncRequestId (les);

    BREthereumLESMessage message = {
        LES_MESSAGE_GET_BLOCK_HEADERS,
        { .getBlockHeaders = messageLESGetBlockHeadersCreate (requestId,
                                                              blockNumber,
                                                              maxBlockCount,
                                                              skip,
                                                              ETHEREUM_BOOLEAN_IS_TRUE (reverse)) }
    };
    
    lesAddReqeust(les, requestId, context, callback, message, LES_PREFERRED_NODE_INDEX);
}

/// MARK: Get Block Bodies

extern void
lesGetBlockBodies (BREthereumLES les,
                   BREthereumLESBlockBodiesContext context,
                   BREthereumLESBlockBodiesCallback callback,
                   BRArrayOf(BREthereumHash) hashes) {
    uint64_t requestId = lesGetThenIncRequestId (les);

    BREthereumLESMessage message = {
            LES_MESSAGE_GET_BLOCK_BODIES,
            { .getBlockBodies = { requestId, hashes }}
    };

    lesAddReqeust(les, requestId, context, callback, message, LES_PREFERRED_NODE_INDEX);
}

extern void
lesGetBlockBodiesOne (BREthereumLES les,
                      BREthereumLESBlockBodiesContext context,
                      BREthereumLESBlockBodiesCallback callback,
                      BREthereumHash hash) {
    BRArrayOf(BREthereumHash) hashes;
    array_new (hashes, 1);
    array_add (hashes, hash);
    lesGetBlockBodies (les, context, callback, hashes);
}

/// MARK: Get Receipts

extern void
lesGetReceipts (BREthereumLES les,
                BREthereumLESReceiptsContext context,
                BREthereumLESReceiptsCallback callback,
                BRArrayOf(BREthereumHash) hashes) {
    uint64_t requestId = lesGetThenIncRequestId (les);

    BREthereumLESMessage message = {
            LES_MESSAGE_GET_RECEIPTS,
            { .getReceipts = { requestId, hashes }}
    };

    lesAddReqeust(les, requestId, context, callback, message, LES_PREFERRED_NODE_INDEX);
}

extern void
lesGetReceiptsOne (BREthereumLES les,
                   BREthereumLESReceiptsContext context,
                   BREthereumLESReceiptsCallback callback,
                   BREthereumHash hash) {

    BRArrayOf(BREthereumHash) hashes;
    array_new (hashes, 1);
    array_add (hashes, hash);
    lesGetReceipts (les, context, callback, hashes);
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

    uint64_t requestId = lesGetThenIncRequestId (les);

    BRArrayOf(BREthereumLESMessageGetProofsSpec) proofSpecs;
    array_new (proofSpecs, 1);

    BRRlpItem key1Item = addressRlpEncode (address, les->coder);
    BRRlpItem key2Item = addressRlpEncode (address, les->coder);

    BREthereumLESMessageGetProofsSpec proofSpec = {
        blockHash,
        rlpGetData (les->coder, key1Item),
        rlpGetData (les->coder, key2Item),
        0,
        blockNumber,
        address
    };
    array_add (proofSpecs, proofSpec);

    rlpReleaseItem (les->coder, key1Item);
    rlpReleaseItem (les->coder, key2Item);

    BREthereumLESMessage message = {
            LES_MESSAGE_GET_PROOFS_V2,
            { .getProofsV2 = { requestId, proofSpecs }}
    };

    // A ProofsV2 message w/ AccountState callbacks....
    lesAddReqeust (les, requestId, context, callback, message, LES_PREFERRED_NODE_INDEX);
}

/// MARK: Get Proofs V2

extern void
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
    BREthereumLESMessage message = {
            LES_MESSAGE_GET_PROOFS_V2,
            { .getProofs = { requestId, specs }}
    };

    lesAddReqeust(les, requestId, context, callback, message, LES_PREFERRED_NODE_INDEX);
}

/// MARK: Get Transaction Status

extern void
lesGetTransactionStatus (BREthereumLES les,
                         BREthereumLESTransactionStatusContext context,
                         BREthereumLESTransactionStatusCallback callback,
                         BRArrayOf(BREthereumHash) transactions) {
    uint64_t requestId = lesGetThenIncRequestId (les);

    BREthereumLESMessage message = {
            LES_MESSAGE_GET_TX_STATUS,
            { .getTxStatus = { requestId, transactions }}
    };

    lesAddReqeust(les, requestId, context, callback, message, LES_PREFERRED_NODE_INDEX);
}

extern void
lesGetTransactionStatusOne (BREthereumLES les,
                            BREthereumLESTransactionStatusContext context,
                            BREthereumLESTransactionStatusCallback callback,
                            BREthereumHash transaction) {

    BRArrayOf(BREthereumHash) transactions;
    array_new (transactions, 1);
    array_add (transactions, transaction);
    lesGetTransactionStatus (les, context, callback, transactions);
}

/// MARK: Submit Transactions

static void
lesSubmitTransactions (BREthereumLES les,
                       BREthereumLESTransactionStatusContext context,
                       BREthereumLESTransactionStatusCallback callback,
                       BRArrayOf (BREthereumTransaction) transactions) {

    // Generate LES_SUBMIT_TRANSACTION_COUNT submissions - to different nodes.
    for (size_t nodeIndex = 0; nodeIndex < LES_SUBMIT_TRANSACTION_COUNT; nodeIndex++) {
        uint64_t requestId = lesGetThenIncRequestId (les);

        BREthereumLESMessage message = {
                LES_MESSAGE_SEND_TX2,
                { .sendTx2 = { requestId, transactions }}
        };

        lesAddReqeust(les, requestId, context, callback, message, nodeIndex);
    }
}

extern void
lesSubmitTransaction (BREthereumLES les,
                      BREthereumLESTransactionStatusContext context,
                      BREthereumLESTransactionStatusCallback callback,
                      BREthereumTransaction transaction) {
    BRArrayOf(BREthereumTransaction) transactions;
    array_new (transactions, 1);
    array_add (transactions, transaction);
    lesSubmitTransactions (les, context, callback, transactions);
}

