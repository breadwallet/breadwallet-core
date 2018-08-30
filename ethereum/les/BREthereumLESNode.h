//
//  BREthereumLESNode.h
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

#ifndef BR_Ethereum_LES_Node_H
#define BR_Ethereum_LES_Node_H

#include "BREthereumLESMessage.h"
#include "BREthereumLESNodeEndpoint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumLESNodeRecord *BREthereumLESNode;

typedef enum {
    NODE_AVAILABLE,
    NODE_CONNECTING,
    NODE_CONNECTED,
    NODE_EXHAUSTED,
    NODE_ERROR_UNIX,
    NODE_ERROR_DISCONNECT,
    NODE_ERROR_PROTOCOL
} BREthereumLESNodeStateType;

typedef enum  {
    NODE_CONNECT_OPEN,

    NODE_CONNECT_AUTH,
    NODE_CONNECT_AUTH_ACK,
    NODE_CONNECT_HELLO,
    NODE_CONNECT_HELLO_ACK,
    NODE_CONNECT_STATUS,
    NODE_CONNECT_STATUS_ACK,

    NODE_CONNECT_PING,
    NODE_CONNECT_PING_ACK,
} BREthereumLESNodeConnectType;

typedef enum {
    NODE_PROTOCOL_NONSTANDARD_PORT,
    NODE_PROTOCOL_UDP_PING_PONG_MISSED,
    NODE_PROTOCOL_UDP_EXCESSIVE_BYTE_COUNT,
    NODE_PROTOCOL_TCP_AUTHENTICATION,
    NODE_PROTOCOL_TCP_HELLO_MISSED,
    NODE_PROTOCOL_TCP_STATUS_MISSED,
    NODE_PROTOCOL_CAPABILITIES_MISMATCH
} BREEthereumLESNodeProtocolReason;

typedef struct {
    BREthereumLESNodeStateType type;
    union {
        struct {
            BREthereumLESNodeConnectType type;
        } connect;

        struct {
            uint64_t timestamp;
        } exhausted;

        struct {
            int error;
        } unix;

        struct {
            BREthereumP2PDisconnectReason reason;
        } disconnect;

        struct {
            BREEthereumLESNodeProtocolReason reason;
        } protocol;
    } u;
} BREthereumLESNodeState;

/** Fills description, returns description */
extern const char *
nodeStateDescribe (const BREthereumLESNodeState *state,
                   char description[128]);

typedef void *BREthereumLESNodeContext;

typedef void
(*BREthereumLESNodeCallbackMessage) (BREthereumLESNodeContext context,
                                     BREthereumLESNode node,
                                     BREthereumLESMessage message);

typedef void
(*BREthereumLESNodeCallbackState) (BREthereumLESNodeContext context,
                                   BREthereumLESNode node,
                                   BREthereumLESNodeEndpointRoute route,
                                   BREthereumLESNodeState state);

// connect
// disconnect
// network reachable

typedef enum {
    NODE_TYPE_GETH,
    NODE_TYPE_PARITY
} BREthereumLESNodeType;

/**
 * Create a node
 *
 * @param network
 * @param remote
 * @param remote
 * @param local
 * @param context
 * @param callbackMessage
 * @param callbackStatus
 * @return
 */
extern BREthereumLESNode // add 'message id offset'?
nodeCreate (BREthereumNetwork network,
            BREthereumLESNodeEndpoint remote,  // remote, local ??
            BREthereumLESNodeEndpoint local,
            BREthereumLESNodeContext context,
            BREthereumLESNodeCallbackMessage callbackMessage,
            BREthereumLESNodeCallbackState callbackStatus);

extern void
nodeRelease (BREthereumLESNode node);

extern void
nodeConnect (BREthereumLESNode node,
             BREthereumLESNodeEndpointRoute route);

extern void
nodeDisconnect (BREthereumLESNode node,
                BREthereumLESNodeEndpointRoute route,
                BREthereumP2PDisconnectReason reason);

extern BREthereumLESNodeEndpoint *
nodeGetRemoteEndpoint (BREthereumLESNode node);

extern BREthereumLESNodeEndpoint *
nodeGetLocalEndpoint (BREthereumLESNode node);

extern int
nodeHasState (BREthereumLESNode node,
              BREthereumLESNodeEndpointRoute route,
              BREthereumLESNodeStateType type);

extern int
nodeHasErrorState (BREthereumLESNode node,
                   BREthereumLESNodeEndpointRoute route);

extern BREthereumLESNodeState
nodeGetState (BREthereumLESNode node,
              BREthereumLESNodeEndpointRoute route);

extern void
nodeSetStateErrorProtocol (BREthereumLESNode node,
                           BREthereumLESNodeEndpointRoute route,
                           BREEthereumLESNodeProtocolReason reason);

extern int
nodeUpdateDescriptors (BREthereumLESNode node,
                       fd_set *read,
                       fd_set *write);

extern int
nodeCanProcess (BREthereumLESNode node,
                BREthereumLESNodeEndpointRoute route,
                fd_set *descriptors);

extern uint64_t
nodeEstimateCredits (BREthereumLESNode node,
                     BREthereumMessage message);

extern uint64_t
nodeGetCredits (BREthereumLESNode node);

extern BREthereumBoolean
nodeGetDiscovered (BREthereumLESNode node);

extern void
nodeSetDiscovered (BREthereumLESNode node,
                   BREthereumBoolean discovered);

extern size_t
nodeHashValue (const void *node);

extern int
nodeHashEqual (const void *node1,
               const void *node2);

typedef enum {
    NODE_STATUS_SUCCESS,
    NODE_STATUS_ERROR
} BREthereumLESNodeStatus;

typedef struct {
    BREthereumLESNodeStatus status;
    union {
        struct {
            BREthereumMessage message;
        } success;

        struct {
        } error;
    } u;
} BREthereumLESNodeMessageResult;

extern BREthereumLESNodeStatus
nodeSend (BREthereumLESNode node,
          BREthereumLESNodeEndpointRoute route,
          BREthereumMessage message);   // BRRlpData/BRRlpItem *optionalMessageData/Item

extern BREthereumLESNodeMessageResult
nodeRecv (BREthereumLESNode node,
          BREthereumLESNodeEndpointRoute route);

extern void
nodeShow (BREthereumLESNode node);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Node_H */
