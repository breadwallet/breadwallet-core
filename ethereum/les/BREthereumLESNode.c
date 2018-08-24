//
//  BREthereumLESNodeX.c
//  Core
//
//  Created by Ed Gamble on 8/13/18.
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

#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "BRCrypto.h"
#include "BRKeyECIES.h"
#include "BREthereumLESNode.h"
#include "BREthereumLESFrameCoder.h"

// #define NEED_TO_PRINT_SEND_RECV_DATA
#define PTHREAD_STACK_SIZE (512 * 1024)
#define PTHREAD_NULL   ((pthread_t) NULL)

#define DEFAULT_SEND_DATA_BUFFER_SIZE   (16 * 1024)
#define DEFAULT_RECV_DATA_BUFFER_SIZE   (1 * 1024 * 1024)

#if defined (__ANDROID__)
#include "../event/pthread_android.h"
#endif

//
// Frame Coder Stuff
//
#define SIG_SIZE_BYTES      65
#define PUBLIC_SIZE_BYTES   64
#define HEPUBLIC_BYTES      32
#define NONCE_BYTES         32

static const ssize_t authBufLen = SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES + NONCE_BYTES + 1;
static const ssize_t authCipherBufLen =  authBufLen + 65 + 16 + 32;

static const ssize_t ackBufLen = PUBLIC_SIZE_BYTES + NONCE_BYTES + 1;
static const ssize_t ackCipherBufLen =  ackBufLen + 65 + 16 + 32;

/* Forward Declarations */
typedef void* (*ThreadRoutine) (void*);

static void *
nodeThreadConnectUDP (BREthereumLESNode node);

static void *
nodeThreadConnectTCP (BREthereumLESNode node);

//
static void _sendAuthInitiator(BREthereumLESNode node);
static void _readAuthFromInitiator(BREthereumLESNode node);
static void _sendAuthAckToInitiator(BREthereumLESNode node);
static void _readAuthAckFromRecipient(BREthereumLESNode node);

//static void
//sleepForSure (unsigned int seconds, int print) {
//    while (seconds > 0) {
//        if (print) printf ("***\n*** SLEEPING: %d\n", seconds);
//        seconds = sleep(seconds);
//    }
//}

typedef enum  {
    NODE_STATE_DISCONNECTED,
    NODE_STATE_OPEN,

    NODE_STATE_AUTH,
    NODE_STATE_AUTH_ACK,
    NODE_STATE_HELLO,
    NODE_STATE_HELLO_ACK,
    NODE_STATE_STATUS,
    NODE_STATE_STATUS_ACK,

    NODE_STATE_PING,
    NODE_STATE_PING_ACK,

    NODE_STATE_CONNECTED,
    NODE_STATE_ERROR
    // ...
} BREthereumLESNodeState;

typedef enum {
    NODE_TYPE_GETH,
    NODE_TYPE_PARITY
} BREthereumLESNodeType;

//
// MARK: - LES Node
//
struct BREthereumLESNodeRecord {
    BREthereumLESNodeType type;

    BREthereumLESNodeState states[NUMBER_OF_NODE_ROUTES];

    BREthereumLESNodeEndpoint remote;
    BREthereumLESNodeEndpoint local;

    // Message Limits (e.g. 192 header, 32 account state)

    uint64_t requestId;
    uint64_t messageIdOffset;
    uint64_t credits;

    // Pending Request/Response

    // Callbacks
    BREthereumLESNodeContext callbackContext;
    BREthereumLESNodeCallbackMessage callbackMessage;
    BREthereumLESNodeCallbackConnect callbackConnect;

    // Send/Recv Buffer
    BRRlpData sendDataBuffer;
    BRRlpData recvDataBuffer;

    // Message Coder
    BREthereumMessageCoder coder;

    // Frame Coder
    BREthereumLESFrameCoder frameCoder;
    uint8_t authBuf[authBufLen];
    uint8_t authBufCipher[authCipherBufLen];
    uint8_t ackBuf[ackBufLen];
    uint8_t ackBufCipher[ackCipherBufLen];

    //
    // pthread
    //
    char *threadName;
    pthread_t threads[NUMBER_OF_NODE_ROUTES];
    pthread_mutex_t lock;
};

extern BREthereumLESNode
nodeCreate (BREthereumNetwork network,
            BREthereumLESNodeEndpoint remote,  // remote, local ??
            BREthereumLESNodeEndpoint local,
            BREthereumLESNodeContext context,
            BREthereumLESNodeCallbackMessage callbackMessage,
            BREthereumLESNodeCallbackConnect callbackConnect) {
    BREthereumLESNode node = calloc (1, sizeof (struct BREthereumLESNodeRecord));

    node->type   = NODE_TYPE_GETH;

    for (int route = 0; route < NUMBER_OF_NODE_ROUTES; route++)
        node->states[route] = NODE_STATE_DISCONNECTED;

    node->remote = remote;
    node->local  = local;

    node->requestId = 0;
    node->messageIdOffset = 0x10;
    node->credits = 0;

    node->sendDataBuffer = (BRRlpData) { DEFAULT_SEND_DATA_BUFFER_SIZE, malloc (DEFAULT_SEND_DATA_BUFFER_SIZE) };
    node->recvDataBuffer = (BRRlpData) { DEFAULT_RECV_DATA_BUFFER_SIZE, malloc (DEFAULT_RECV_DATA_BUFFER_SIZE) };

    node->coder.network = network;
    node->coder.rlp = rlpCoderCreate();

    node->frameCoder = frameCoderCreate();
    frameCoderInit(node->frameCoder,
                   &remote.ephemeralKey, &remote.nonce,
                   &local.ephemeralKey,  &local.nonce,
                   node->ackBufCipher, ackCipherBufLen,
                   node->authBufCipher, authCipherBufLen,
                   ETHEREUM_BOOLEAN_TRUE);

    node->callbackContext = context;
    node->callbackMessage = callbackMessage;
    node->callbackConnect = callbackConnect;

    {
#define PTHREAD_NAME_BASE    "Core Ethereum LES"
        char threadName[4096];
        sprintf (threadName, "%s %s", PTHREAD_NAME_BASE, node->remote.hostname);
        node->threadName = strdup(threadName);

        for (int route = 0; route < NUMBER_OF_NODE_ROUTES; route++)
            node->threads[route] = NULL;

        // The cacheLock is a normal, non-recursive lock
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        pthread_mutex_init(&node->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    return node;
}

extern void
nodeRelease (BREthereumLESNode node) {
    nodeDisconnect (node, NODE_ROUTE_TCP);
    nodeDisconnect (node, NODE_ROUTE_UDP);

    if (NULL != node->sendDataBuffer.bytes) free (node->sendDataBuffer.bytes);
    if (NULL != node->recvDataBuffer.bytes) free (node->recvDataBuffer.bytes);

    rlpCoderRelease(node->coder.rlp);
    frameCoderRelease(node->frameCoder);

    free (node);
}

extern BREthereumLESNodeEndpoint *
nodeGetRemoteEndpoint (BREthereumLESNode node) {
    return &node->remote;
}

extern BREthereumLESNodeEndpoint *
nodeGetLocalEndpoint (BREthereumLESNode node) {
    return &node->local;
}

extern void
nodeConnect (BREthereumLESNode node,
             BREthereumLESNodeEndpointRoute route) {
    pthread_mutex_lock (&node->lock);
    if (PTHREAD_NULL == node->threads[route]) {
        pthread_attr_t attr;
        pthread_attr_init (&attr);
        pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setstacksize (&attr, PTHREAD_STACK_SIZE);

        pthread_create (&node->threads[route], &attr,
                        (ThreadRoutine) (NODE_ROUTE_TCP == route ? nodeThreadConnectTCP : nodeThreadConnectUDP),
                        node);
        pthread_attr_destroy (&attr);
    }
    pthread_mutex_unlock (&node->lock);
}

extern void
nodeDisconnect (BREthereumLESNode node,
                BREthereumLESNodeEndpointRoute route) {
    pthread_mutex_lock (&node->lock);

    // Cancel the thread, if it exists.
    if (PTHREAD_NULL != node->threads[route]) {
        pthread_cancel (node->threads[route]);
        pthread_join (node->threads[route], NULL);
        node->threads[route] = PTHREAD_NULL;
    }

    // Close the appropriate endpoint route
    nodeEndpointClose (&node->remote, route);
    node->states[route] = NODE_STATE_DISCONNECTED;

    pthread_mutex_unlock (&node->lock);
}

extern int
nodeIsConnected (BREthereumLESNode node,
                 BREthereumLESNodeEndpointRoute route) {
    return NODE_STATE_CONNECTED == node->states[route];
}

extern int
nodeIsConnecting (BREthereumLESNode node,
                  BREthereumLESNodeEndpointRoute route) {
    return (NODE_STATE_DISCONNECTED != node->states[route] &&
            NODE_STATE_CONNECTED    != node->states[route]);
}

static int maximum (int x, int y) { return x > y ? x : y; }

extern int
nodeUpdateDescriptors (BREthereumLESNode node,
                       fd_set *read,
                       fd_set *write) {
    if (nodeIsConnected (node, NODE_ROUTE_TCP)) {
        int socket = node->remote.sockets[NODE_ROUTE_TCP];
        if (socket != -1 && NULL != read)  FD_SET (socket, read);
        if (socket != -1 && NULL != write) FD_SET (socket, write);
    }

    if (nodeIsConnected (node, NODE_ROUTE_UDP)) {
        int socket = node->remote.sockets[NODE_ROUTE_UDP];
        if (socket != -1 && NULL != read)  FD_SET (socket, read);
        if (socket != -1 && NULL != write) FD_SET (socket, write);
    }

    return maximum (node->remote.sockets[NODE_ROUTE_TCP],
                    node->remote.sockets[NODE_ROUTE_UDP]);
}

extern int
nodeCanProcess (BREthereumLESNode node,
                BREthereumLESNodeEndpointRoute route,
                fd_set *descriptors) {
    return (nodeIsConnected (node, route) &&
            NULL != descriptors &&
            FD_ISSET (node->remote.sockets[route], descriptors));
}

static void *
nodeFailed (BREthereumLESNode node) {
    node->callbackConnect (node->callbackContext, node, NODE_ERROR);
    pthread_mutex_unlock (&node->lock);
    return NULL;
}

//
//
//
static void *
nodeThreadConnectUDP (BREthereumLESNode node) {
#if defined (__ANDROID__)
    pthread_setname_np (node->thread, node->threadName);
#else
    pthread_setname_np (node->threadName);
#endif
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);

    int error = 0;

    pthread_mutex_lock (&node->lock);

    assert (NODE_STATE_DISCONNECTED == node->states[NODE_ROUTE_UDP]);

    {
        // OPEN
        node->states[NODE_ROUTE_UDP] = NODE_STATE_OPEN;
        error = nodeEndpointOpen (&node->remote, NODE_ROUTE_UDP);
        if (error) return nodeFailed (node);
    }

    BREthereumMessage message;

    //
    // PING
    //
    node->states[NODE_ROUTE_UDP] = NODE_STATE_PING;
    message = (BREthereumMessage) {
        MESSAGE_DIS,
        { .dis = {
            DIS_MESSAGE_PING,
            { .ping = messageDISPingCreate (node->local.dis, // endpointDISCreate(&node->local),
                                            node->remote.dis, // endpointDISCreate(&node->remote),
                                            time(NULL) + 1000000) },
            node->local.key }}
    };
    nodeSend (node, NODE_ROUTE_UDP, message);

    //
    // PING_ACK
    //
    message = nodeRecv (node, NODE_ROUTE_UDP);
    if (MESSAGE_DIS != message.identifier || DIS_MESSAGE_PONG != message.u.dis.identifier)
        return nodeFailed (node);

    message = nodeRecv (node, NODE_ROUTE_UDP);
    if (MESSAGE_DIS != message.identifier || DIS_MESSAGE_PING != message.u.dis.identifier)
        return nodeFailed (node);

    message = (BREthereumMessage) {
        MESSAGE_DIS,
        { .dis = {
            DIS_MESSAGE_PONG,
            { .pong =
                messageDISPongCreate (message.u.dis.u.ping.to,
                                      message.u.dis.u.ping.hash,
                                      time(NULL) + 1000000) },
            nodeGetLocalEndpoint(node)->key }}
    };
    nodeSend (node, NODE_ROUTE_UDP, message);

    //
    // CONNECTED
    //
    node->states[NODE_ROUTE_UDP] = NODE_STATE_CONNECTED;
    node->callbackConnect (node->callbackContext, node, NODE_SUCCESS);
    pthread_mutex_unlock (&node->lock);
    return NULL;
}

//
//
//
static void *
nodeThreadConnectTCP (BREthereumLESNode node) {
#if defined (__ANDROID__)
    pthread_setname_np (node->thread, node->threadName);
#else
    pthread_setname_np (node->threadName);
#endif
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);

    int error = 0;

    pthread_mutex_lock (&node->lock);

    assert (NODE_STATE_DISCONNECTED == node->states[NODE_ROUTE_TCP]);

    {
        // OPEN
        node->states[NODE_ROUTE_TCP] = NODE_STATE_OPEN;
        error = nodeEndpointOpen (&node->remote, NODE_ROUTE_TCP);
        if (error) return nodeFailed (node);
    }

    BREthereumMessage message;

    //
    // AUTH
    //
    node->states[NODE_ROUTE_TCP] = NODE_STATE_AUTH;

    _sendAuthInitiator(node);
    eth_log (LES_LOG_TOPIC, "Send: [ WIP, %15s ] => %s", "Auth",    node->remote.hostname);
    nodeEndpointSendData (&node->remote, NODE_ROUTE_TCP, node->authBufCipher, authCipherBufLen); //  "auth initiator");

    //
    // AUTH_ACK
    //
    node->states[NODE_ROUTE_TCP] = NODE_STATE_AUTH_ACK;
    size_t ackCipherBufCount = ackCipherBufLen;
    nodeEndpointRecvData (&node->remote, NODE_ROUTE_TCP, node->ackBufCipher, &ackCipherBufCount, 1); // "auth ack from receivier"
    eth_log (LES_LOG_TOPIC, "Recv: [ WIP, %15s ] <= %s", "Auth Ack",    node->remote.hostname);
    assert (ackCipherBufCount == ackCipherBufLen);

    _readAuthAckFromRecipient (node);

    // Initilize the frameCoder with the information from the auth
    frameCoderInit(node->frameCoder,
                   &node->remote.ephemeralKey, &node->remote.nonce,
                   &node->local.ephemeralKey, &node->local.nonce,
                   node->ackBufCipher, ackCipherBufLen,
                   node->authBufCipher, authCipherBufLen,
                   ETHEREUM_BOOLEAN_TRUE);

    //
    // HELLO
    //
    node->states[NODE_ROUTE_TCP] = NODE_STATE_HELLO;
    nodeSend (node, NODE_ROUTE_TCP, (BREthereumMessage) {
        MESSAGE_P2P,
        { .p2p = node->local.hello }
    });

    //
    // HELLO ACK
    //
    node->states[NODE_ROUTE_TCP] = NODE_STATE_HELLO_ACK;
    message = nodeRecv (node, NODE_ROUTE_TCP);
    assert (MESSAGE_P2P == message.identifier);
    assert (P2P_MESSAGE_HELLO == message.u.p2p.identifier);

    // Save the 'hello' message received and then move on
    messageP2PHelloShow (message.u.p2p.u.hello);
    node->remote.hello = message.u.p2p;

    // A P2P message, not a LES message
    //            if (node->callbackMessage)
    //                node->callbackMessage (node->callbackContext,
    //                                       node,
    //                                       message.u.les);

    //
    // STATUS
    //
    node->states[NODE_ROUTE_TCP] = NODE_STATE_STATUS;
    nodeSend (node, NODE_ROUTE_TCP, (BREthereumMessage) {
        MESSAGE_LES,
        { .les = node->local.status }
    });

    //
    // STATUS_ACK
    //
    node->states[NODE_ROUTE_TCP] = NODE_STATE_STATUS_ACK;
    message = nodeRecv (node, NODE_ROUTE_TCP);
    assert (MESSAGE_LES == message.identifier);
    assert (LES_MESSAGE_STATUS == message.u.les.identifier);

    // Save the 'status' message received and then move on
    messageLESStatusShow(&message.u.les.u.status);
    node->remote.status = message.u.les;

    if (node->callbackMessage)
        node->callbackMessage (node->callbackContext,
                               node,
                               message.u.les);


    //
    // CONNECTED
    //
    node->states[NODE_ROUTE_TCP] = NODE_STATE_CONNECTED;
    node->callbackConnect (node->callbackContext, node, NODE_SUCCESS);
    pthread_mutex_unlock (&node->lock);
    return NULL;
}


//            case NODE_STATE_CONNECTED: {
//                fd_set readFds;
//                int numFds = nodeEndpointGetRecvDataAvailableFDSNum (&node->remote);
//
//                nodeEndpointSetRecvDataAvailableFDS (&node->remote, &readFds);
//                pselect (numFds, &readFds, NULL, NULL, NULL, NULL);
//
//                int socket = (-1 != node->remote.socketTCP && FD_ISSET (node->remote.socketTCP, &readFds)
//                              ? node->remote.socketTCP
//                              : node->remote.socketUDP);
//                //
//                BREthereumMessage message = nodeRecv (node); // fd_set(), select() in nodeRecv()
//
//
//                // If this is a P2P DISCONNECT message, then disconnect; otherwise ...
//                if (messageHasIdentifiers (&message, MESSAGE_P2P, P2P_MESSAGE_DISCONNECT)) {
//                    eth_log (LES_LOG_TOPIC, "Disconnected: %d\n", message.u.p2p.u.disconnect.reason);
//                    node->state = NODE_STATE_DISCONNECTED;
//                }
//
//                // ... if this is a P2P PING message, respond immediately; otherwise ...
//                else if (messageHasIdentifiers (&message, MESSAGE_P2P, P2P_MESSAGE_PING)) {
//                    BREthereumMessage pong = {
//                        MESSAGE_P2P,
//                        { .p2p = {
//                            P2P_MESSAGE_PONG,
//                            {}}}
//                    };
//                    nodeSend (node, pong);
//                }
//
//                // ... if this is a DIS PING message, respond immediately; otherwise ...
//                else if (messageHasIdentifiers (&message, MESSAGE_DIS, DIS_MESSAGE_PING)) {
//                    // Send PONG so we are 'bonded' and ...
//                    BREthereumMessage pong = {
//                        MESSAGE_DIS,
//                        { .dis = {
//                            DIS_MESSAGE_PONG,
//                            { .pong =
//                                messageDISPongCreate (message.u.dis.u.ping.to,
//                                                      message.u.dis.u.ping.hash,
//                                                      time(NULL) + 1000000) },
//                            node->local.key }}
//                    };
//                    nodeSend (node, pong);
//
//                    // ... can then send a 'findNodes'
//                    BREthereumMessage findNodes = {
//                        MESSAGE_DIS,
//                        { .dis = {
//                            DIS_MESSAGE_FIND_NEIGHBORS,
//                            { .findNeighbors =
//                                messageDISFindNeighborsCreate (node->remote.key,
//                                                               time(NULL) + 1000000) },
//                            node->local.key }}
//                    };
//                    nodeSend (node, findNodes);
//                }
//
//                // .. if this is a DIS PONG message, respond immediately; otherwise ...
//                else if (messageHasIdentifiers (&message, MESSAGE_DIS, DIS_MESSAGE_PONG)) {
//                }
//
//                // ... handle the message
//                else if (messageHasIdentifier(&message, MESSAGE_LES)) {
//                    // if a les message, extends a 'reqId result' and then, if complete
//                    // announce it.
//                    if (node->callbackMessage)
//                        node->callbackMessage (node->callbackContext,
//                                               node,
//                                               message.u.les);
//                }
//                break;
//            }


/**
 * Send `message` on `route` to `node`.  There is a consistency constraint whereby the message
 * identifier must be MESSAGE_DIS if and only if route is UDP.
 *
 * @param node
 * @param route
 * @param message
 */
extern void
nodeSend (BREthereumLESNode node,
          BREthereumLESNodeEndpointRoute route,
           BREthereumMessage message) {

    assert ((NODE_ROUTE_UDP == route && MESSAGE_DIS == message.identifier) ||
            (NODE_ROUTE_UDP != route && MESSAGE_DIS != message.identifier));

    BRRlpItem item = messageEncode (message, node->coder);

    eth_log (LES_LOG_TOPIC, "Send: [ %s, %15s ] => %s",
             messageGetIdentifierName (&message),
             messageGetAnyIdentifierName (&message),
             node->remote.hostname);

    // Handle DIS messages specially.
    switch (message.identifier) {
        case MESSAGE_DIS: {
            // Extract the `item` bytes w/o the RLP length prefix.  This ends up being
            // simply the raw bytes.  We *know* the `item` is an RLP encoding of bytes; thus we
            // use `rlpDecodeBytes` (rather than `rlpDecodeList`.  Then simply send them.
            BRRlpData data = rlpDecodeBytesSharedDontRelease (node->coder.rlp, item);

#if defined (NEED_TO_PRINT_SEND_RECV_DATA)
            eth_log (LES_LOG_TOPIC, "Size: Send: %s: PayLoad: %zu",
                     nodeEndpointRouteGetName(route),
                     data.bytesCount);
#endif
            nodeEndpointSendData (&node->remote, route, data.bytes, data.bytesCount);
            break;
        }
        default: {
            // Extract the `items` bytes w/o the RLP length prefix.  We *know* the `item` is an
            // RLP encoding of a list; thus we use `rlpDecodeList`.
            BRRlpData data = rlpDecodeListSharedDontRelease(node->coder.rlp, item);

            // Encrypt the length-less data
            BRRlpData encryptedData;
            frameCoderEncrypt(node->frameCoder,
                              data.bytes, data.bytesCount,
                              &encryptedData.bytes, &encryptedData.bytesCount);

#if defined (NEED_TO_PRINT_SEND_RECV_DATA)
            eth_log (LES_LOG_TOPIC, "Size: Send: %s: PayLoad: %zu",
                     nodeEndpointRouteGetName(route),
                     encryptedData.bytesCount);
#endif
            nodeEndpointSendData (&node->remote, route, encryptedData.bytes, encryptedData.bytesCount);
            break;
        }
    }
    rlpReleaseItem (node->coder.rlp, item);
}

//static uint8_t  // somehow
//messageOffsets[4] = {
//    0x00,   // MESSAGE_P2P
//    0x00,   // MESSAGE_ETH,
//    0x10,   // MESSAGE_LES,
//    0x00,   // MESSAGE_DIS
//};

static void
extractIdentifier (uint8_t value,
                   BREthereumMessageIdentifier *type,
                   BREthereumANYMessageIdentifier *subtype) {
    if (value >= LES_IDENTIFIER_OFFSET_DEAL_WITH_IT) {
        *type = MESSAGE_LES;
        *subtype = value - LES_IDENTIFIER_OFFSET_DEAL_WITH_IT;
    }
    else {
        *type = MESSAGE_P2P;
        *subtype = value - 0x00;
    }
}

extern BREthereumMessage
nodeRecv (BREthereumLESNode node,
          BREthereumLESNodeEndpointRoute route) {
    uint8_t *bytes = node->recvDataBuffer.bytes;
    size_t   bytesLimit = node->recvDataBuffer.bytesCount;
    size_t   bytesCount = 0;

    BREthereumMessage message;

    switch (route) {
        case NODE_ROUTE_UDP: {
            bytesCount = 1500;

            assert (0 == nodeEndpointRecvData (&node->remote, route, bytes, &bytesCount, 0));
            //    assert (0 == nodeEndpointRecvData (&node->remote, NULL, bytes, bytesLimit));

            // Wrap at RLP Byte
            BRRlpItem item = rlpEncodeBytes (node->coder.rlp, bytes, bytesCount);

            message = messageDecode (item, node->coder,
                                     MESSAGE_DIS,
                                     (BREthereumDISMessageIdentifier) -1);
            rlpReleaseItem (node->coder.rlp, item);
            break;
        }

        case NODE_ROUTE_TCP: {
            size_t headerCount = 32;

            {
                // get header, decrypt it, validate it and then determine the bytesCpount
                uint8_t header[32];
                memset(header, -1, 32);

                assert (0 == nodeEndpointRecvData (&node->remote, route, header, &headerCount, 1));
                assert (ETHEREUM_BOOLEAN_IS_TRUE(frameCoderDecryptHeader(node->frameCoder, header, 32)));
                headerCount = ((uint32_t)(header[2]) <<  0 |
                               (uint32_t)(header[1]) <<  8 |
                               (uint32_t)(header[0]) << 16);

                // ??round to 16 ?? 32 ??
                bytesCount = headerCount + ((16 - (headerCount % 16)) % 16) + 16;
                // bytesCount = (headerCount + 15) & ~15;

                // ?? node->bodySize = headerCount; ??
            }

            // Given bytesCount, update recvDataBuffer if too small
            if (bytesCount > bytesLimit) {
                node->recvDataBuffer.bytesCount = bytesCount;
                node->recvDataBuffer.bytes = realloc(node->recvDataBuffer.bytes, bytesCount);
                bytes = node->recvDataBuffer.bytes;
                bytesLimit = bytesCount;
            }

#if defined (NEED_TO_PRINT_SEND_RECV_DATA)
            eth_log (LES_LOG_TOPIC, "Size: Recv: TCP: PayLoad: %u, Frame: %zu", headerCount, bytesCount);
#endif
            
            // get body/frame
            assert (0 == nodeEndpointRecvData (&node->remote, route, bytes, &bytesCount, 1));
            frameCoderDecryptFrame(node->frameCoder, bytes, bytesCount);

            // ?? node->bodySize = headerCount; ??

            // Identifier is at byte[0]
            BRRlpData identifierData = { 1, &bytes[0] };
            BRRlpItem identifierItem = rlpGetItem (node->coder.rlp, identifierData);
            uint8_t value = (uint8_t) rlpDecodeUInt64 (node->coder.rlp, identifierItem, 1);

            BREthereumMessageIdentifier type;
            BREthereumANYMessageIdentifier subtype;

            extractIdentifier(value, &type, &subtype);

            // Actual body
            BRRlpData data = { headerCount - 1, &bytes[1] };
            BRRlpItem item = rlpGetItem (node->coder.rlp, data);

#if defined (NEED_TO_PRINT_SEND_RECV_DATA)
            eth_log (LES_LOG_TOPIC, "Size: Recv: TCP: Type: %u, Subtype: %d", type, subtype);
#endif

            message = messageDecode (item, node->coder, type, subtype);

            rlpReleaseItem (node->coder.rlp, item);
            rlpReleaseItem (node->coder.rlp, identifierItem);

            break;
        }
    }

    eth_log (LES_LOG_TOPIC, "Recv: [ %s, %15s ] <= %s",
             messageGetIdentifierName (&message),
             messageGetAnyIdentifierName (&message),
             node->remote.hostname);


    return message;
}

static void
bytesXOR(uint8_t * op1, uint8_t* op2, uint8_t* result, size_t len) {
    for (unsigned int i = 0; i < len;  ++i) {
        result[i] = op1[i] ^ op2[i];
    }
}

static void
_BRECDH(void *out32, const BRKey *privKey, BRKey *pubKey)
{
    uint8_t p[65];
    size_t pLen = BRKeyPubKey(pubKey, p, sizeof(p));

    if (pLen == 65) p[0] = (p[64] % 2) ? 0x03 : 0x02; // convert to compressed pubkey format
    BRSecp256k1PointMul((BRECPoint *)p, &privKey->secret); // calculate shared secret ec-point
    memcpy(out32, &p[1], 32); // unpack the x coordinate

    mem_clean(p, sizeof(p));
}


static void
_sendAuthInitiator(BREthereumLESNode node) {

    // eth_log(LES_LOG_TOPIC, "%s", "generating auth initiator");

    // authInitiator -> E(remote-pubk, S(ephemeral-privk, static-shared-secret ^ nonce) || H(ephemeral-pubk) || pubk || nonce || 0x0)
    uint8_t * authBuf = node->authBuf;
    uint8_t * authBufCipher = node->authBufCipher;

    uint8_t* signature = &authBuf[0];
    uint8_t* hPubKey = &authBuf[SIG_SIZE_BYTES];
    uint8_t* pubKey = &authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES];
    uint8_t* nonce =  &authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES];
    BRKey* nodeKey   = &node->local.key;   //nodeGetKey(node);
    BRKey* remoteKey = &node->remote.key;  // nodeGetPeerKey(node);

    //static-shared-secret = ecdh.agree(privkey, remote-pubk)
    UInt256 staticSharedSecret;
    _BRECDH(staticSharedSecret.u8, nodeKey, remoteKey);

    //static-shared-secret ^ nonce
    UInt256 xorStaticNonce;
    UInt256* localNonce = &node->local.nonce;       // nodeGetNonce(node);
    BRKey* localEphemeral = &node->local.ephemeralKey; //  nodeGetEphemeral(node);
    memset(xorStaticNonce.u8, 0, 32);
    bytesXOR(staticSharedSecret.u8, localNonce->u8, xorStaticNonce.u8, sizeof(localNonce->u8));


    // S(ephemeral-privk, static-shared-secret ^ nonce)
    // Determine the signature length
    size_t signatureLen = 65; BRKeyCompactSignEthereum(localEphemeral,
                                                       NULL, 0,
                                                       xorStaticNonce);

    // Fill the signature
    signatureLen = BRKeyCompactSignEthereum(localEphemeral,
                                            signature, signatureLen,
                                            xorStaticNonce);

    // || H(ephemeral-pubk)||
    memset(&hPubKey[32], 0, 32);
    uint8_t ephPublicKey[65];
    BRKeyPubKey(localEphemeral, ephPublicKey, 65);
    BRKeccak256(hPubKey, &ephPublicKey[1], PUBLIC_SIZE_BYTES);
    // || pubK ||
    uint8_t nodePublicKey[65] = {0};
    BRKeyPubKey(nodeKey, nodePublicKey, 65);
    memcpy(pubKey, &nodePublicKey[1], PUBLIC_SIZE_BYTES);
    // || nonce ||
    memcpy(nonce, localNonce->u8, sizeof(localNonce->u8));
    // || 0x0   ||
    authBuf[authBufLen - 1] = 0x0;

    // E(remote-pubk, S(ephemeral-privk, static-shared-secret ^ nonce) || H(ephemeral-pubk) || pubk || nonce || 0x0)
    BRKeyECIESAES128SHA256Encrypt(remoteKey, authBufCipher, authCipherBufLen, localEphemeral, authBuf, authBufLen);
}

static void
_readAuthFromInitiator(BREthereumLESNode node) {
    BRKey* nodeKey = &node->local.key; // nodeGetKey(node);
    eth_log (LES_LOG_TOPIC, "%s", "received auth from initiator");

    size_t len = BRKeyECIESAES128SHA256Decrypt(nodeKey, node->authBuf, authBufLen, node->authBufCipher, authCipherBufLen);

    if (len != authBufLen) {
        //TODO: call _readAuthFromInitiatorEIP8...
    }
    else {
        //copy remote nonce
        UInt256* remoteNonce = &node->remote.nonce; // nodeGetPeerNonce(node);
        memcpy(remoteNonce->u8, &node->authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES], sizeof(remoteNonce->u8));

        //copy remote public key
        uint8_t remotePubKey[65];
        remotePubKey[0] = 0x04;
        BRKey* remoteKey = &node->remote.key; // nodeGetPeerKey(node);
        remoteKey->compressed = 0;
        memcpy(&remotePubKey[1], &node->authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES], PUBLIC_SIZE_BYTES);
        BRKeySetPubKey(remoteKey, remotePubKey, 65);

        UInt256 sharedSecret;
        _BRECDH(sharedSecret.u8, nodeKey, remoteKey);

        UInt256 xOrSharedSecret;
        bytesXOR(sharedSecret.u8, remoteNonce->u8, xOrSharedSecret.u8, sizeof(xOrSharedSecret.u8));

        // The ephemeral public key of the remote peer
        BRKey* remoteEphemeral = &node->remote.ephemeralKey; // nodeGetPeerEphemeral(node);
        BRKeyRecoverPubKeyEthereum(remoteEphemeral, xOrSharedSecret, node->authBuf, SIG_SIZE_BYTES);
    }
}


static void
_sendAuthAckToInitiator(BREthereumLESNode node) {
    eth_log (LES_LOG_TOPIC, "%s", "generating auth ack for initiator");

    // authRecipient -> E(remote-pubk, epubK|| nonce || 0x0)
    uint8_t* ackBuf = node->ackBuf;
    uint8_t* ackBufCipher = node->ackBufCipher;
    BRKey* remoteKey = &node->remote.key; // nodeGetPeerKey(node);

    uint8_t* pubKey = &ackBuf[0];
    uint8_t* nonce =  &ackBuf[PUBLIC_SIZE_BYTES];

    // || epubK ||
    uint8_t localEphPublicKey[65];
    BRKey* localEphemeral = &node->local.ephemeralKey; // nodeGetEphemeral(node);
    size_t ephPubKeyLength = BRKeyPubKey(localEphemeral, localEphPublicKey, 65);
    assert(ephPubKeyLength == 65);
    memcpy(pubKey, &localEphPublicKey[1], 64);

    // || nonce ||
    UInt256* localNonce = &node->local.nonce; // nodeGetNonce(node);
    memcpy(nonce, localNonce->u8, sizeof(localNonce->u8));
    // || 0x0   ||
    ackBuf[ackBufLen- 1] = 0x0;

    //E(remote-pubk, epubK || nonce || 0x0)
    BRKeyECIESAES128SHA256Encrypt(remoteKey, ackBufCipher, ackCipherBufLen, localEphemeral, ackBuf, ackBufLen);

}

static void
_readAuthAckFromRecipient(BREthereumLESNode node) {

    BRKey* nodeKey = &node->local.key; // nodeGetKey(node);

    // eth_log (LES_LOG_TOPIC,"%s", "received auth ack from recipient");

    size_t len = BRKeyECIESAES128SHA256Decrypt(nodeKey, node->ackBuf, ackBufLen, node->ackBufCipher, ackCipherBufLen);

    if (len != ackBufLen) {
        //TODO: call _readAckAuthFromRecipientEIP8...
        eth_log (LES_LOG_TOPIC, "%s", "Something went wrong with AUK");
        assert(1);
    }
    else {
        //copy remote nonce key
        UInt256* nonce = &node->remote.nonce; // nodeGetPeerNonce(node);
        memcpy(nonce->u8, &node->ackBuf[PUBLIC_SIZE_BYTES], sizeof(nonce->u8));

        //copy ephemeral public key of the remote peer
        uint8_t remoteEPubKey[65];
        remoteEPubKey[0] = 0x04;
        BRKey* remoteEphemeral = &node->remote.ephemeralKey; // nodeGetPeerEphemeral(node);
        memcpy(&remoteEPubKey[1], node->ackBuf, PUBLIC_SIZE_BYTES);
        BRKeySetPubKey(remoteEphemeral, remoteEPubKey, 65);
    }
}
