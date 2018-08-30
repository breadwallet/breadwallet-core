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
#include <errno.h>
#include "BRCrypto.h"
#include "BRKeyECIES.h"
#include "BREthereumLESNode.h"
#include "BREthereumLESFrameCoder.h"

// #define NEED_TO_PRINT_SEND_RECV_DATA
#define NEED_TO_AVOID_PROOFS_LOGGING

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
static int _sendAuthInitiator(BREthereumLESNode node);
static int _readAuthAckFromRecipient(BREthereumLESNode node);

static inline int maximum (int x, int y) { return x > y ? x : y; }

//static void
//sleepForSure (unsigned int seconds, int print) {
//    while (seconds > 0) {
//        if (print) printf ("***\n*** SLEEPING: %d\n", seconds);
//        seconds = sleep(seconds);
//    }
//}

/// MARK: LES Node State Create ...

static inline BREthereumLESNodeState
nodeStateCreate (BREthereumLESNodeStateType type) {
    return (BREthereumLESNodeState) { type };
}

static BREthereumLESNodeState
nodeStateCreateAvailable (void) {
    return nodeStateCreate (NODE_AVAILABLE);
}

static BREthereumLESNodeState
nodeStateCreateConnecting (BREthereumLESNodeConnectType type) {
    return (BREthereumLESNodeState) {
        NODE_CONNECTING,
        { .connect = { type }}
    };
}

static BREthereumLESNodeState
nodeStateCreateConnected (void) {
    return nodeStateCreate (NODE_CONNECTED);
}

static BREthereumLESNodeState
nodeStateCreateExhausted (uint64_t timestamp) {
    return (BREthereumLESNodeState) {
        NODE_EXHAUSTED,
        { .exhausted = { timestamp }}
    };
}

static BREthereumLESNodeState
nodeStateCreateErrorUnix (int error) {
    return (BREthereumLESNodeState) {
        NODE_ERROR_UNIX,
        { .connect = { error }}
    };
}

static BREthereumLESNodeState
nodeStateCreateErrorDisconnect (BREthereumP2PDisconnectReason reason) {
    return (BREthereumLESNodeState) {
        NODE_ERROR_DISCONNECT,
        { .disconnect = { reason }}
    };
}

static BREthereumLESNodeState
nodeStateCreateErrorProtocol (BREEthereumLESNodeProtocolReason reason) {
    return (BREthereumLESNodeState) {
        NODE_ERROR_PROTOCOL,
        { .protocol = { reason }}
    };
}

const char *
nodeProtocolReasonDescription (BREEthereumLESNodeProtocolReason reason) {
    static const char *
    protocolReasonDescriptions [] = {
        "Non-Standard Port",
        "UDP Ping_Pong Missed",
        "UDP Excessive Byte Count",
        "TCP Authentication",
        "TCP Hello Missed",
        "TCP Status Missed",
        "Capabilities Mismatch",
    };
    return protocolReasonDescriptions [reason];
}

extern const char *
nodeStateDescribe (const BREthereumLESNodeState *state,
                   char description[128]) {
    switch (state->type) {
        case NODE_AVAILABLE:  return strcpy (description, "Available");
        case NODE_CONNECTING: return strcpy (description, "Connecting");
        case NODE_CONNECTED:  return strcpy (description, "Connected");
        case NODE_EXHAUSTED:  return strcpy (description, "Exhausted");
        case NODE_ERROR_UNIX:       return strcat (strcpy (description, "Unix: "),
                                                   strerror (state->u.unix.error));
        case NODE_ERROR_DISCONNECT: return strcat (strcpy (description, "Disconnect: "),
                                                   messageP2PDisconnectDescription(state->u.disconnect.reason));
        case NODE_ERROR_PROTOCOL:   return strcat (strcpy (description, "Protocol: "),
                                                   nodeProtocolReasonDescription(state->u.protocol.reason));
    }
}

//
// MARK: - LES Node
//
struct BREthereumLESNodeRecord {
    // Must be first to support BRSet.
    /**
     * The identifier is the 'nodeId' from the remote endpoint - which is itself the 64 byte
     * publicKey for the endpoint.
     */
    UInt512 identifier;

    /** The type as GETH or PARITY (only GETH supported) */
    BREthereumLESNodeType type;

    /** The states by route; one for UDP and one for TCP */
    BREthereumLESNodeState states[NUMBER_OF_NODE_ROUTES];

    // The endpoints connected by this node
    BREthereumLESNodeEndpoint local;
    BREthereumLESNodeEndpoint remote;

    /** The message specs by identifier.  Includes credit params and message count limits */
    BREthereumLESMessageSpec specs [NUMBER_OF_LES_MESSAGE_IDENTIFIERS];

    /** Credit remaining (if not zero) */
    uint64_t credits;

    /** Callbacks */
    BREthereumLESNodeContext callbackContext;
    BREthereumLESNodeCallbackMessage callbackMessage;
    BREthereumLESNodeCallbackState callbackStatus;

    /** Send/Recv Buffer */
    BRRlpData sendDataBuffer;
    BRRlpData recvDataBuffer;

    /** Message Coder - remember 'not thread safe'! */
    BREthereumMessageCoder coder;

    BREthereumBoolean discovered;

    /** Frame Coder */
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


//
// Create
//
extern BREthereumLESNode
nodeCreate (BREthereumNetwork network,
            BREthereumLESNodeEndpoint remote,  // remote, local ??
            BREthereumLESNodeEndpoint local,
            BREthereumLESNodeContext context,
            BREthereumLESNodeCallbackMessage callbackMessage,
            BREthereumLESNodeCallbackState callbackStatus) {
    BREthereumLESNode node = calloc (1, sizeof (struct BREthereumLESNodeRecord));

    // Extract the identifier from the remote's public key.
    memcpy (node->identifier.u8, &remote.key.pubKey[1], 64);

    // Fixed the type as GETH (for now, at least).
    node->type = NODE_TYPE_GETH;

    // Make all routes as 'available'
    for (int route = 0; route < NUMBER_OF_NODE_ROUTES; route++)
        node->states[route] = nodeStateCreateAvailable();

    // Save the local and remote nodes.
    node->local  = local;
    node->remote = remote;

    // Fill in the specs with default values (for GETH)
    for (int i = 0; i < NUMBER_OF_LES_MESSAGE_IDENTIFIERS; i++)
        node->specs[i] = messageLESSpecs[i];

    // No credits, yet.
    node->credits = 0;

    node->sendDataBuffer = (BRRlpData) { DEFAULT_SEND_DATA_BUFFER_SIZE, malloc (DEFAULT_SEND_DATA_BUFFER_SIZE) };
    node->recvDataBuffer = (BRRlpData) { DEFAULT_RECV_DATA_BUFFER_SIZE, malloc (DEFAULT_RECV_DATA_BUFFER_SIZE) };

    // Define the message coder
    node->coder.network = network;
    node->coder.rlp = rlpCoderCreate();
    node->coder.lesMessageIdOffset = 0x00;  // Changed with 'hello' message exchange.

    node->discovered = ETHEREUM_BOOLEAN_FALSE;

    node->frameCoder = frameCoderCreate();
    frameCoderInit(node->frameCoder,
                   &remote.ephemeralKey, &remote.nonce,
                   &local.ephemeralKey,  &local.nonce,
                   node->ackBufCipher, ackCipherBufLen,
                   node->authBufCipher, authCipherBufLen,
                   ETHEREUM_BOOLEAN_TRUE);

    node->callbackContext = context;
    node->callbackMessage = callbackMessage;
    node->callbackStatus = callbackStatus;

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
    nodeDisconnect (node, NODE_ROUTE_TCP, P2P_MESSAGE_DISCONNECT_REQUESTED);
    nodeDisconnect (node, NODE_ROUTE_UDP, P2P_MESSAGE_DISCONNECT_REQUESTED);

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

extern size_t
nodeHashValue (const void *node) {
    // size_t varies by platform (32 or 64 bits).
    return (size_t) ((BREthereumLESNode) node)->identifier.u64[0];
}

extern int
nodeHashEqual (const void *node1,
               const void *node2) {
    return UInt512Eq (((BREthereumLESNode) node1)->identifier,
                      ((BREthereumLESNode) node2)->identifier);
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
                BREthereumLESNodeEndpointRoute route,
                BREthereumP2PDisconnectReason reason) {
    pthread_mutex_lock (&node->lock);

    // Cancel the thread, if it exists.
    if (PTHREAD_NULL != node->threads[route]) {
        pthread_cancel (node->threads[route]);
        pthread_join (node->threads[route], NULL);
        node->threads[route] = PTHREAD_NULL;
    }

    // Close the appropriate endpoint route
    nodeEndpointClose (&node->remote, route,
                       (P2P_MESSAGE_DISCONNECT_REQUESTED == reason &&
                        !nodeHasErrorState(node, route)));

    switch (node->states[route].type) {
        case NODE_ERROR_UNIX:
        case NODE_ERROR_DISCONNECT:
        case NODE_ERROR_PROTOCOL:
        case NODE_EXHAUSTED:
            // If the current state is an 'error-ish' state, then don't modify the state
            break;

        case NODE_CONNECTING:
        case NODE_CONNECTED:
            // otherwise, return to 'available' if the disconnet is requested.
            node->states[route] = (P2P_MESSAGE_DISCONNECT_REQUESTED == reason
                                   ? nodeStateCreateAvailable()
                                   : nodeStateCreateErrorDisconnect(reason));
            break;

        default:
            break;
    }
    pthread_mutex_unlock (&node->lock);
}

/**
 * Extract the `type` and `subtype` of a message from the RLP-encoded `value`.  The `value` has
 * any applicable messagerIdOffset applied; thus we need to undo that offset.
 *
 * We've already assumed that we have one subprotocol (LES, PIP) and thus one and only one
 * offset to deal with.
 */
static void
extractIdentifier (BREthereumLESNode node,
                   uint8_t value,
                   BREthereumMessageIdentifier *type,
                   BREthereumANYMessageIdentifier *subtype) {
    if (value < node->coder.lesMessageIdOffset || 0 == node->coder.lesMessageIdOffset) {
        *type = MESSAGE_P2P;
        *subtype = value - 0x00;
    }
    else {
        *type = MESSAGE_LES;
        *subtype = value - node->coder.lesMessageIdOffset;
    }
}

/// MARK: LES Node State

static void
nodeStateAnnounce (BREthereumLESNode node,
                   BREthereumLESNodeEndpointRoute route,
                   BREthereumLESNodeState state) {
    node->states [route] = state;
    node->callbackStatus (node->callbackContext, node, route, state);
}

extern int
nodeHasState (BREthereumLESNode node,
              BREthereumLESNodeEndpointRoute route,
              BREthereumLESNodeStateType type) {
    return type == node->states[route].type;
}

extern int
nodeHasErrorState (BREthereumLESNode node,
                   BREthereumLESNodeEndpointRoute route) {
    switch (node->states[route].type) {
        case NODE_AVAILABLE:
        case NODE_CONNECTING:
        case NODE_CONNECTED:
            return 0;
        case NODE_EXHAUSTED:
        case NODE_ERROR_UNIX:
        case NODE_ERROR_DISCONNECT:
        case NODE_ERROR_PROTOCOL:
            return 1;
    }
}

extern BREthereumLESNodeState
nodeGetState (BREthereumLESNode node,
              BREthereumLESNodeEndpointRoute route) {
    return node->states[route];
}

extern void
nodeSetStateErrorProtocol (BREthereumLESNode node,
                           BREthereumLESNodeEndpointRoute route,
                           BREEthereumLESNodeProtocolReason reason) {
    node->states[route] = nodeStateCreateErrorProtocol(reason);
}

/// MARK: Descriptors

extern int
nodeUpdateDescriptors (BREthereumLESNode node,
                       fd_set *read,
                       fd_set *write) {
    if (nodeHasState(node, NODE_ROUTE_TCP, NODE_CONNECTED)) {
        int socket = node->remote.sockets[NODE_ROUTE_TCP];
        if (socket != -1 && NULL != read)  FD_SET (socket, read);
        if (socket != -1 && NULL != write) FD_SET (socket, write);
    }

    if (nodeHasState(node, NODE_ROUTE_UDP, NODE_CONNECTED)) {
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
    return (nodeHasState (node, route, NODE_CONNECTED) &&
            NULL != descriptors &&
            FD_ISSET (node->remote.sockets[route], descriptors));
}

/// MARK: UDP & TCP Connect

/**
 * Clean up any lingering state for a non-local exit.
 */
static void *
nodeConnectExit (BREthereumLESNode node) {
    //    pthread_mutex_unlock (&node->lock);
    pthread_exit (0);
    return NULL;
}

/**
 * Announce the state and then clean up lingering state.
 */
static void *
nodeConnectFailed (BREthereumLESNode node,
                   BREthereumLESNodeEndpointRoute route,
                   BREthereumLESNodeState state) {
    nodeEndpointClose (&node->remote, route, 0);
    nodeStateAnnounce (node, route, state);
    return nodeConnectExit (node);
}

//
//
//
static void *
nodeThreadConnectUDP (BREthereumLESNode node) {
    int error = 0;
    BREthereumLESNodeMessageResult result;
    BREthereumMessage message;

#if defined (__ANDROID__)
    pthread_setname_np (node->thread, node->threadName);
#else
    pthread_setname_np (node->threadName);
#endif
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);

//    pthread_mutex_lock (&node->lock);

    // If the current state is somehow not AVAILABLE, then we've no point continuing.
    if (!nodeHasState (node, NODE_ROUTE_UDP, NODE_AVAILABLE))
        return nodeConnectExit (node);

    // OPEN
    node->states[NODE_ROUTE_UDP] = nodeStateCreateConnecting(NODE_CONNECT_OPEN);
    error = nodeEndpointOpen (&node->remote, NODE_ROUTE_UDP);
    if (error) return nodeConnectFailed (node, NODE_ROUTE_UDP, nodeStateCreateErrorUnix(error));

    int socket = node->remote.sockets[NODE_ROUTE_UDP];
    fd_set readSet, writeSet;
    struct timespec timeout = { 1, 0 }; // 1 second
    FD_ZERO (&readSet); FD_ZERO (&writeSet);
    FD_SET (socket, &readSet); FD_SET (socket, &writeSet);
    errno = 0;

    //
    // PING
    //
    node->states[NODE_ROUTE_UDP] = nodeStateCreateConnecting(NODE_CONNECT_PING);
    message = (BREthereumMessage) {
        MESSAGE_DIS,
        { .dis = {
            DIS_MESSAGE_PING,
            { .ping = messageDISPingCreate (node->local.dis, // endpointDISCreate(&node->local),
                                            node->remote.dis, // endpointDISCreate(&node->remote),
                                            time(NULL) + 1000000) },
            node->local.key }}
    };
    if (NODE_STATUS_ERROR == nodeSend (node, NODE_ROUTE_UDP, message))
        return nodeConnectExit (node);

    //
    // PING_ACK
    //
    node->states[NODE_ROUTE_UDP] = nodeStateCreateConnecting(NODE_CONNECT_PING_ACK);

    error = pselect (socket + 1, &readSet, NULL, NULL, &timeout, NULL);
    if (error <= 0)
        return nodeConnectFailed (node, NODE_ROUTE_UDP,
                                  nodeStateCreateErrorUnix (error == 0 ? ETIMEDOUT : errno));
    result = nodeRecv (node, NODE_ROUTE_UDP);
    if (NODE_STATUS_ERROR == result.status)
        return nodeConnectExit (node);

    // Require a PONG message
    message = result.u.success.message;
    if (MESSAGE_DIS != message.identifier || DIS_MESSAGE_PONG != message.u.dis.identifier)
        return nodeConnectFailed (node, NODE_ROUTE_UDP, nodeStateCreateErrorProtocol(NODE_PROTOCOL_UDP_PING_PONG_MISSED));

    error = pselect (socket + 1, &readSet, NULL, NULL, &timeout, NULL);
    if (error <= 0)
        return nodeConnectFailed (node, NODE_ROUTE_UDP,
                                  nodeStateCreateErrorUnix (error == 0 ? ETIMEDOUT : errno));
    result = nodeRecv (node, NODE_ROUTE_UDP);
    if (NODE_STATUS_ERROR == result.status)
        return nodeConnectExit (node);

    // Require a PING message
    message = result.u.success.message;
    if (MESSAGE_DIS != message.identifier || DIS_MESSAGE_PING != message.u.dis.identifier)
        return nodeConnectFailed (node, NODE_ROUTE_UDP, nodeStateCreateErrorProtocol(NODE_PROTOCOL_UDP_PING_PONG_MISSED));

    // Respond with PONG
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
    if (NODE_STATUS_ERROR == nodeSend (node, NODE_ROUTE_UDP, message))
        return nodeConnectExit (node);

    //
    // CONNECTED
    //
    nodeStateAnnounce (node, NODE_ROUTE_UDP, nodeStateCreateConnected());
    return nodeConnectExit (node);   // pthread_mutex_unlock (&node->lock);
}

//
//
//
static void *
nodeThreadConnectTCP (BREthereumLESNode node) {
    int error = 0;
    BREthereumLESNodeMessageResult result;
    BREthereumMessage message;

#if defined (__ANDROID__)
    pthread_setname_np (node->thread, node->threadName);
#else
    pthread_setname_np (node->threadName);
#endif
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype  (PTHREAD_CANCEL_DEFERRED, NULL);

//    pthread_mutex_lock (&node->lock);

    if (!nodeHasState (node, NODE_ROUTE_TCP, NODE_AVAILABLE))
        return nodeConnectExit (node);

    // OPEN
    node->states[NODE_ROUTE_TCP] = nodeStateCreateConnecting(NODE_CONNECT_OPEN);
    error = nodeEndpointOpen (&node->remote, NODE_ROUTE_TCP);
    if (error) return nodeConnectFailed (node, NODE_ROUTE_TCP, nodeStateCreateErrorUnix(error));

    int socket = node->remote.sockets[NODE_ROUTE_TCP];
    fd_set readSet, writeSet;
    struct timespec timeout = { 1, 0 }; // 1 second
    FD_ZERO (&readSet); FD_ZERO (&writeSet);
    FD_SET (socket, &readSet); FD_SET (socket, &writeSet);
    errno = 0;

    //
    // AUTH
    //
    node->states[NODE_ROUTE_TCP] = nodeStateCreateConnecting(NODE_CONNECT_AUTH);

    if (0 != _sendAuthInitiator(node))
        return nodeConnectFailed (node, NODE_ROUTE_TCP, nodeStateCreateErrorProtocol(NODE_PROTOCOL_TCP_AUTHENTICATION));
    eth_log (LES_LOG_TOPIC, "Send: [ WIP, %15s ] => %s", "Auth",    node->remote.hostname);

    error = nodeEndpointSendData (&node->remote, NODE_ROUTE_TCP, node->authBufCipher, authCipherBufLen); //  "auth initiator");
    if (error) return nodeConnectFailed (node, NODE_ROUTE_TCP, nodeStateCreateErrorUnix(error));

    //
    // AUTH_ACK
    //
    node->states[NODE_ROUTE_TCP] = nodeStateCreateConnecting(NODE_CONNECT_AUTH_ACK);
    size_t ackCipherBufCount = ackCipherBufLen;

    error = pselect (socket + 1, &readSet, NULL, NULL, &timeout, NULL);
    if (error <= 0)
        return nodeConnectFailed (node, NODE_ROUTE_TCP,
                                  nodeStateCreateErrorUnix (error == 0 ? ETIMEDOUT : errno));
    error = nodeEndpointRecvData (&node->remote, NODE_ROUTE_TCP, node->ackBufCipher, &ackCipherBufCount, 1); // "auth ack from receivier"
    if (error) return nodeConnectFailed (node, NODE_ROUTE_TCP, nodeStateCreateErrorUnix(error));

    eth_log (LES_LOG_TOPIC, "Recv: [ WIP, %15s ] <= %s", "Auth Ack",    node->remote.hostname);
    if (ackCipherBufCount != ackCipherBufLen)
        return nodeConnectFailed (node, NODE_ROUTE_TCP, nodeStateCreateErrorProtocol(NODE_PROTOCOL_TCP_AUTHENTICATION));

    if (0 != _readAuthAckFromRecipient (node)) {
        eth_log (LES_LOG_TOPIC, "%s", "Something went wrong with AUK");
        return nodeConnectFailed (node, NODE_ROUTE_TCP, nodeStateCreateErrorProtocol(NODE_PROTOCOL_TCP_AUTHENTICATION));
    }

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
    node->states[NODE_ROUTE_TCP] = nodeStateCreateConnecting(NODE_CONNECT_HELLO);
    message = (BREthereumMessage) {
        MESSAGE_P2P,
        { .p2p = node->local.hello }
    };
    if (NODE_STATUS_ERROR == nodeSend (node, NODE_ROUTE_TCP, message))
        return nodeConnectExit (node);

    //
    // HELLO ACK
    //
    node->states[NODE_ROUTE_TCP] = nodeStateCreateConnecting(NODE_CONNECT_HELLO_ACK);
    error = pselect (socket + 1, &readSet, NULL, NULL, &timeout, NULL);
    if (error <= 0)
        return nodeConnectFailed (node, NODE_ROUTE_TCP,
                                  nodeStateCreateErrorUnix (error == 0 ? ETIMEDOUT : errno));
    result = nodeRecv (node, NODE_ROUTE_TCP);
    if (NODE_STATUS_ERROR == result.status) return NULL;

    message = result.u.success.message;

    // Handle a disconnect request
    if (MESSAGE_P2P == message.identifier && P2P_MESSAGE_DISCONNECT == message.u.p2p.identifier)
        return nodeConnectFailed(node, NODE_ROUTE_TCP, nodeStateCreateErrorDisconnect(message.u.p2p.u.disconnect.reason));

    // Require a P2P Hello message.
    if (MESSAGE_P2P != message.identifier || P2P_MESSAGE_HELLO != message.u.p2p.identifier)
        return nodeConnectFailed (node, NODE_ROUTE_TCP, nodeStateCreateErrorProtocol(NODE_PROTOCOL_TCP_HELLO_MISSED));

    // Save the 'hello' message received and then move on
    messageP2PHelloShow (message.u.p2p.u.hello);
    node->remote.hello = message.u.p2p;

    // Confirm that the remote has all the local capabilities.
    int capabilitiesMatch = 1;
    {
        BREthereumP2PMessageHello *localHello  = &node->local.hello.u.hello;
        BREthereumP2PMessageHello *remoteHello = &node->remote.hello.u.hello;
        for (size_t li = 0; li < array_count(localHello->capabilities); li++)
            capabilitiesMatch &= ETHEREUM_BOOLEAN_IS_TRUE (messageP2PHelloHasCapability
                                                           (remoteHello,
                                                            &localHello->capabilities[li]));
    }
    if (! capabilitiesMatch)
        return nodeConnectFailed(node, NODE_ROUTE_TCP, nodeStateCreateErrorProtocol(NODE_PROTOCOL_CAPABILITIES_MISMATCH));

    // https://github.com/ethereum/wiki/wiki/ÐΞVp2p-Wire-Protocol
    // ÐΞVp2p is designed to support arbitrary sub-protocols (aka capabilities) over the basic wire
    // protocol. Each sub-protocol is given as much of the message-ID space as it needs (all such
    // protocols must statically specify how many message IDs they require). On connection and
    // reception of the Hello message, both peers have equivalent information about what
    // subprotocols they share (including versions) and are able to form consensus over the
    // composition of message ID space.
    //
    // Message IDs are assumed to be compact from ID 0x10 onwards (0x00-0x10 is reserved for
    // ÐΞVp2p messages) and given to each shared (equal-version, equal name) sub-protocol in
    // alphabetic order. Sub-protocols that are not shared are ignored. If multiple versions are
    // shared of the same (equal name) sub-protocol, the numerically highest wins, others are
    // ignored

    // We'll trust (but verify) that we have one and only one (LES, PIP) subprotocol.
    assert (1 == array_count(node->local.hello.u.hello.capabilities));
    node->coder.lesMessageIdOffset = 0x10;

    // A P2P message, not a LES message
    //            if (node->callbackMessage)
    //                node->callbackMessage (node->callbackContext,
    //                                       node,
    //                                       message.u.les);

    //
    // STATUS
    //
    node->states[NODE_ROUTE_TCP] = nodeStateCreateConnecting(NODE_CONNECT_STATUS);
    message = (BREthereumMessage) {
        MESSAGE_LES,
        { .les = node->local.status }
    };
    if (NODE_STATUS_ERROR == nodeSend (node, NODE_ROUTE_TCP, message))
        return nodeConnectExit (node);

    //
    // STATUS_ACK
    //
    node->states[NODE_ROUTE_TCP] = nodeStateCreateConnecting(NODE_CONNECT_STATUS_ACK);
    error = pselect (socket + 1, &readSet, NULL, NULL, &timeout, NULL);
    if (error <= 0)
        return nodeConnectFailed (node, NODE_ROUTE_TCP,
                                  nodeStateCreateErrorUnix (error == 0 ? ETIMEDOUT : errno));
    result = nodeRecv (node, NODE_ROUTE_TCP);
    if (NODE_STATUS_ERROR == result.status)
        return nodeConnectExit (node);

    message = result.u.success.message;

    // Handle a disconnect request
    if (MESSAGE_P2P == message.identifier && P2P_MESSAGE_DISCONNECT == message.u.p2p.identifier)
        return nodeConnectFailed(node, NODE_ROUTE_TCP, nodeStateCreateErrorDisconnect(message.u.p2p.u.disconnect.reason));

    // Require a LES Status message.
    if (MESSAGE_LES != message.identifier || LES_MESSAGE_STATUS != message.u.les.identifier)
        return nodeConnectFailed (node, NODE_ROUTE_TCP, nodeStateCreateErrorProtocol(NODE_PROTOCOL_TCP_STATUS_MISSED));

    // Save the 'status' message
    messageLESStatusShow(&message.u.les.u.status);
    node->remote.status = message.u.les;

    // Extract the per message cost parameters (from the status MRC data)
    BREthereumLESMessageStatus *status = &message.u.les.u.status;
    if (NULL != status->flowControlMRCCount)
        for (int i = 0; i < *status->flowControlMRCCount; i++) {
            BREthereumLESMessageStatusMRC mrc = status->flowControlMRC[i];
            if (mrc.msgCode < NUMBER_OF_LES_MESSAGE_IDENTIFIERS) {
                node->specs[mrc.msgCode].baseCost = mrc.baseCost;
                node->specs[mrc.msgCode].reqCost  = mrc.reqCost;
            }
        }

    // Announce the LES Status message.
    if (node->callbackMessage)
        node->callbackMessage (node->callbackContext,
                               node,
                               message.u.les);

    //
    // CONNECTED
    //
    nodeStateAnnounce (node, NODE_ROUTE_TCP, nodeStateCreateConnected());
    return nodeConnectExit (node);      // pthread_mutex_unlock (&node->lock);
}

/// MARK: Send

static BREthereumLESNodeStatus
nodeSendFailed (BREthereumLESNode node,
                BREthereumLESNodeEndpointRoute route,
                BREthereumLESNodeState state) {
    nodeStateAnnounce (node, route, state);
    return NODE_STATUS_ERROR;
}

/**
 * Send `message` on `route` to `node`.  There is a consistency constraint whereby the message
 * identifier must be MESSAGE_DIS if and only if route is UDP.
 *
 * @param node
 * @param route
 * @param message
 */
extern BREthereumLESNodeStatus
nodeSend (BREthereumLESNode node,
          BREthereumLESNodeEndpointRoute route,
          BREthereumMessage message) {

    int error = 0;
    size_t bytesCount = 0;

    assert ((NODE_ROUTE_UDP == route && MESSAGE_DIS == message.identifier) ||
            (NODE_ROUTE_UDP != route && MESSAGE_DIS != message.identifier));

    BRRlpItem item = messageEncode (message, node->coder);

#if defined (NEED_TO_AVOID_PROOFS_LOGGING)
    if (MESSAGE_LES != message.identifier || LES_MESSAGE_GET_PROOFS_V2 != message.u.les.identifier)
#endif
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

            pthread_mutex_lock (&node->lock);
            error = nodeEndpointSendData (&node->remote, route, data.bytes, data.bytesCount);
            pthread_mutex_unlock (&node->lock);
            bytesCount = data.bytesCount;
            break;
        }

        default: {
            // Extract the `items` bytes w/o the RLP length prefix.  We *know* the `item` is an
            // RLP encoding of a list; thus we use `rlpDecodeList`.
            BRRlpData data = rlpDecodeListSharedDontRelease(node->coder.rlp, item);

            // Encrypt the length-less data
            BRRlpData encryptedData;
            pthread_mutex_lock (&node->lock);
            frameCoderEncrypt(node->frameCoder,
                              data.bytes, data.bytesCount,
                              &encryptedData.bytes, &encryptedData.bytesCount);

            error = nodeEndpointSendData (&node->remote, route, encryptedData.bytes, encryptedData.bytesCount);
            pthread_mutex_unlock (&node->lock);
            bytesCount = encryptedData.bytesCount;
            break;
        }
    }
    rlpReleaseItem (node->coder.rlp, item);

#if defined (NEED_TO_PRINT_SEND_RECV_DATA)
    if (!error)
        eth_log (LES_LOG_TOPIC, "Size: Send: %s: PayLoad: %zu",
                 nodeEndpointRouteGetName(route),
                 data.bytesCount);
#endif

    return (0 == error
            ? NODE_STATUS_SUCCESS
            : nodeSendFailed (node, route, nodeStateCreateErrorUnix (error)));
}

/// MARK: Recv

static BREthereumLESNodeMessageResult
nodeRecvFailed (BREthereumLESNode node,
                BREthereumLESNodeEndpointRoute route,
                BREthereumLESNodeState state) {
    nodeStateAnnounce (node, route, state);
    return (BREthereumLESNodeMessageResult) { NODE_STATUS_ERROR };
}

extern BREthereumLESNodeMessageResult
nodeRecv (BREthereumLESNode node,
          BREthereumLESNodeEndpointRoute route) {
    uint8_t *bytes = node->recvDataBuffer.bytes;
    size_t   bytesLimit = node->recvDataBuffer.bytesCount;
    size_t   bytesCount = 0;
    int error;

    BREthereumMessage message;

    switch (route) {
        case NODE_ROUTE_UDP: {
            bytesCount = 1500;

            error = nodeEndpointRecvData (&node->remote, route, bytes, &bytesCount, 0);
            if (error) return nodeRecvFailed (node, NODE_ROUTE_UDP, nodeStateCreateErrorUnix (error));
            if (bytesCount > 1500)
                return nodeRecvFailed(node, NODE_ROUTE_UDP,
                                      nodeStateCreateErrorProtocol(NODE_PROTOCOL_UDP_EXCESSIVE_BYTE_COUNT));

            // Wrap at RLP Byte
            BRRlpItem item = rlpEncodeBytes (node->coder.rlp, bytes, bytesCount);

            message = messageDecode (item, node->coder,
                                     MESSAGE_DIS,
                                     MESSAGE_DIS_IDENTIFIER_ANY);
            rlpReleaseItem (node->coder.rlp, item);
            break;
        }

        case NODE_ROUTE_TCP: {
            size_t headerCount = 32;

            {
                // get header, decrypt it, validate it and then determine the bytesCpount
                uint8_t header[32];
                memset(header, -1, 32);

                error = nodeEndpointRecvData (&node->remote, route, header, &headerCount, 1);
                if (error) return nodeRecvFailed (node, NODE_ROUTE_TCP, nodeStateCreateErrorUnix (error));

                pthread_mutex_lock (&node->lock);
                assert (ETHEREUM_BOOLEAN_IS_TRUE(frameCoderDecryptHeader(node->frameCoder, header, 32)));
                pthread_mutex_unlock (&node->lock);
                headerCount = ((uint32_t)(header[2]) <<  0 |
                               (uint32_t)(header[1]) <<  8 |
                               (uint32_t)(header[0]) << 16);

                // ??round to 16 ?? 32 ??
                bytesCount = headerCount + ((16 - (headerCount % 16)) % 16) + 16;
                // bytesCount = (headerCount + 15) & ~15;

                // ?? node->bodySize = headerCount; ??
            }

            // Given bytesCount, update recvDataBuffer if too small
            pthread_mutex_lock (&node->lock);
            if (bytesCount > bytesLimit) {
                node->recvDataBuffer.bytesCount = bytesCount;
                node->recvDataBuffer.bytes = realloc(node->recvDataBuffer.bytes, bytesCount);
                bytes = node->recvDataBuffer.bytes;
                bytesLimit = bytesCount;
            }
            pthread_mutex_unlock (&node->lock);

#if defined (NEED_TO_PRINT_SEND_RECV_DATA)
            eth_log (LES_LOG_TOPIC, "Size: Recv: TCP: PayLoad: %u, Frame: %zu", headerCount, bytesCount);
#endif
            
            // get body/frame
            error = nodeEndpointRecvData (&node->remote, route, bytes, &bytesCount, 1);
            if (error) return nodeRecvFailed (node, NODE_ROUTE_TCP, nodeStateCreateErrorUnix (error));

            pthread_mutex_lock (&node->lock);
            frameCoderDecryptFrame(node->frameCoder, bytes, bytesCount);
            pthread_mutex_unlock (&node->lock);

            // ?? node->bodySize = headerCount; ??

            // Identifier is at byte[0]
            BRRlpData identifierData = { 1, &bytes[0] };
            BRRlpItem identifierItem = rlpGetItem (node->coder.rlp, identifierData);
            uint8_t value = (uint8_t) rlpDecodeUInt64 (node->coder.rlp, identifierItem, 1);

            BREthereumMessageIdentifier type;
            BREthereumANYMessageIdentifier subtype;

            extractIdentifier(node, value, &type, &subtype);

            // Actual body
            BRRlpData data = { headerCount - 1, &bytes[1] };
            BRRlpItem item = rlpGetItem (node->coder.rlp, data);

#if defined (NEED_TO_PRINT_SEND_RECV_DATA)
            eth_log (LES_LOG_TOPIC, "Size: Recv: TCP: Type: %u, Subtype: %d", type, subtype);
#endif

            // Finally, decode the message
            message = messageDecode (item, node->coder, type, subtype);

            // If this is a LES response message, then it has credit information.
            if (MESSAGE_LES == message.identifier &&
                messageLESHasUse (&message.u.les, LES_MESSAGE_USE_RESPONSE))
                node->credits = messageLESGetCredits (&message.u.les);
            
            rlpReleaseItem (node->coder.rlp, item);
            rlpReleaseItem (node->coder.rlp, identifierItem);

            break;
        }
    }

#if defined (NEED_TO_AVOID_PROOFS_LOGGING)
    if (MESSAGE_LES != message.identifier || LES_MESSAGE_PROOFS_V2 != message.u.les.identifier)
#endif
    eth_log (LES_LOG_TOPIC, "Recv: [ %s, %15s ] <= %s",
             messageGetIdentifierName (&message),
             messageGetAnyIdentifierName (&message),
             node->remote.hostname);


    return (BREthereumLESNodeMessageResult) {
        NODE_STATUS_SUCCESS,
        { .success = { message }}
    };
}

/// MARK: Credits

extern uint64_t
nodeEstimateCredits (BREthereumLESNode node,
                     BREthereumMessage message) {
    if (MESSAGE_LES != message.identifier) return 0;

    BREthereumLESMessage *lm = &message.u.les;
    size_t count = 0;

    switch (lm->identifier) {
        case LES_MESSAGE_GET_BLOCK_HEADERS:
            count = lm->u.getBlockHeaders.maxHeaders;
            break;

        case LES_MESSAGE_GET_BLOCK_BODIES:
            count = array_count (lm->u.getBlockBodies.hashes);
            break;

        case LES_MESSAGE_GET_RECEIPTS:
            count = array_count(lm->u.getReceipts.hashes);
            break;

        case LES_MESSAGE_GET_PROOFS:
            count = array_count(lm->u.getProofs.specs);
            break;

        case LES_MESSAGE_GET_CONTRACT_CODES:
            count = 0;
            break;

        case LES_MESSAGE_SEND_TX:
            count = array_count(lm->u.sendTx.transactions);
            break;

        case LES_MESSAGE_GET_HEADER_PROOFS:
            count = 0;
            break;

        case LES_MESSAGE_GET_PROOFS_V2:
            count = array_count(lm->u.getProofsV2.specs);
            break;

        case LES_MESSAGE_GET_HELPER_TRIE_PROOFS:
            count = 0;
            break;

        case LES_MESSAGE_SEND_TX2:
            count = array_count(lm->u.sendTx2.transactions);
            break;

        case LES_MESSAGE_GET_TX_STATUS:
            count = array_count(lm->u.getTxStatus.hashes);
            break;

        default:
            count = 0;
    }

    return node->specs[lm->identifier].baseCost + count * node->specs[lm->identifier].reqCost;
}

extern uint64_t
nodeGetCredits (BREthereumLESNode node) {
    return node->credits;
}

extern BREthereumBoolean
nodeGetDiscovered (BREthereumLESNode node) {
    return node->discovered;
}

extern void
nodeSetDiscovered (BREthereumLESNode node,
                   BREthereumBoolean discovered) {
    node->discovered = discovered;
}

extern void
nodeShow (BREthereumLESNode node) {
    char descUDP[128], descTCP[128];
    eth_log (LES_LOG_TOPIC, "Node: %15s", node->remote.hostname);
    eth_log (LES_LOG_TOPIC, "   UDP       : %s", nodeStateDescribe (&node->states[NODE_ROUTE_UDP], descUDP));
    eth_log (LES_LOG_TOPIC, "   TCP       : %s", nodeStateDescribe (&node->states[NODE_ROUTE_TCP], descTCP));
    eth_log (LES_LOG_TOPIC, "   Discovered: %s", (ETHEREUM_BOOLEAN_IS_TRUE(node->discovered) ? "Yes" : "No"));
    eth_log (LES_LOG_TOPIC, "   Credits   : %llu", node->credits);
}

/// MARK: Support

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


static int // 0 on success
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
    return 0;
}

//static void
//_readAuthFromInitiator(BREthereumLESNode node) {
//    BRKey* nodeKey = &node->local.key; // nodeGetKey(node);
//    eth_log (LES_LOG_TOPIC, "%s", "received auth from initiator");
//
//    size_t len = BRKeyECIESAES128SHA256Decrypt(nodeKey, node->authBuf, authBufLen, node->authBufCipher, authCipherBufLen);
//
//    if (len != authBufLen) {
//        //TODO: call _readAuthFromInitiatorEIP8...
//    }
//    else {
//        //copy remote nonce
//        UInt256* remoteNonce = &node->remote.nonce; // nodeGetPeerNonce(node);
//        memcpy(remoteNonce->u8, &node->authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES + PUBLIC_SIZE_BYTES], sizeof(remoteNonce->u8));
//
//        //copy remote public key
//        uint8_t remotePubKey[65];
//        remotePubKey[0] = 0x04;
//        BRKey* remoteKey = &node->remote.key; // nodeGetPeerKey(node);
//        remoteKey->compressed = 0;
//        memcpy(&remotePubKey[1], &node->authBuf[SIG_SIZE_BYTES + HEPUBLIC_BYTES], PUBLIC_SIZE_BYTES);
//        BRKeySetPubKey(remoteKey, remotePubKey, 65);
//
//        UInt256 sharedSecret;
//        _BRECDH(sharedSecret.u8, nodeKey, remoteKey);
//
//        UInt256 xOrSharedSecret;
//        bytesXOR(sharedSecret.u8, remoteNonce->u8, xOrSharedSecret.u8, sizeof(xOrSharedSecret.u8));
//
//        // The ephemeral public key of the remote peer
//        BRKey* remoteEphemeral = &node->remote.ephemeralKey; // nodeGetPeerEphemeral(node);
//        BRKeyRecoverPubKeyEthereum(remoteEphemeral, xOrSharedSecret, node->authBuf, SIG_SIZE_BYTES);
//    }
//}
//
//static void
//_sendAuthAckToInitiator(BREthereumLESNode node) {
//    eth_log (LES_LOG_TOPIC, "%s", "generating auth ack for initiator");
//
//    // authRecipient -> E(remote-pubk, epubK|| nonce || 0x0)
//    uint8_t* ackBuf = node->ackBuf;
//    uint8_t* ackBufCipher = node->ackBufCipher;
//    BRKey* remoteKey = &node->remote.key; // nodeGetPeerKey(node);
//
//    uint8_t* pubKey = &ackBuf[0];
//    uint8_t* nonce =  &ackBuf[PUBLIC_SIZE_BYTES];
//
//    // || epubK ||
//    uint8_t localEphPublicKey[65];
//    BRKey* localEphemeral = &node->local.ephemeralKey; // nodeGetEphemeral(node);
//    size_t ephPubKeyLength = BRKeyPubKey(localEphemeral, localEphPublicKey, 65);
//    assert(ephPubKeyLength == 65);
//    memcpy(pubKey, &localEphPublicKey[1], 64);
//
//    // || nonce ||
//    UInt256* localNonce = &node->local.nonce; // nodeGetNonce(node);
//    memcpy(nonce, localNonce->u8, sizeof(localNonce->u8));
//    // || 0x0   ||
//    ackBuf[ackBufLen- 1] = 0x0;
//
//    //E(remote-pubk, epubK || nonce || 0x0)
//    BRKeyECIESAES128SHA256Encrypt(remoteKey, ackBufCipher, ackCipherBufLen, localEphemeral, ackBuf, ackBufLen);
//
//}

static int // 0 on success
_readAuthAckFromRecipient(BREthereumLESNode node) {

    BRKey* nodeKey = &node->local.key; // nodeGetKey(node);

    // eth_log (LES_LOG_TOPIC,"%s", "received auth ack from recipient");

    size_t len = BRKeyECIESAES128SHA256Decrypt(nodeKey, node->ackBuf, ackBufLen, node->ackBufCipher, ackCipherBufLen);

    if (len != ackBufLen) {
        //TODO: call _readAckAuthFromRecipientEIP8...
        return 1;
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
        return 0;
    }
}
