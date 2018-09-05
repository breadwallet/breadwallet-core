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
#include "BREthereumProvision.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A Node is a proxy for an Ethereum Node.  This Core Ethereum code uses nodes to connect
 * to Ethereum nodes supporting a light client protocol (for GETH, LESv2; for Parity, PIPv1),
 * to send requests, and to recv respones.
 *
 * The Node interface abstracts over the specific light client protocol (LESv2, PIPv1). Thus,
 * the Node interface provides for Status, Block Header, Block Body, Transaction Receipt,
 * Accounts and others - even though, for example LESv2 and PIPv1 offer very different
 * messages interfaces to send/recv Ethereum data.
 *
 * A Node connects two endpoints - a local one and a remote one.    A Node is not connected
 * unless commanded and if the handshake process succeeds - including capatibility between the
 * local and remote nodes.  Specifically the local and remote nodes must share one of the
 * LESv2 or PIPv1 Ethereum supprotocols.
 *
 * There can be two routes to the remote node - a TCP route and an UDP route; these routes are
 * used for different message types (underlying Node interfaces) - the UDP route only supports
 * Node Discovery; TCP supports other messages types for P2P, ETH, LES and PIP.
 *
 * The connection between local and remote endpoints, whether UDP or TCP, uses a Unix socket
 * for send/recv interactions.  The Node interface allows for a select() call w/ read and write
 * file descriptors.  For a write descriptor, the Node must know if data/messages are pending
 * to be sent to a remote endpoint.
 *
 * When connected, a Node announces the extension to the Ethereum block chain.  As this
 * announcement can occur at any time, once connected the select() read descriptor must be
 * set.
 */
typedef struct BREthereumLESNodeRecord *BREthereumLESNode;

/**
 * A Node has a type of either GETH or PARITY.  The node's interface will be implemented
 * according to the particulars of the type.  For GETH, we'll use the LESv2 subprotocol; for
 * Parity we'll use PIPv1.
 */
typedef enum {
    NODE_TYPE_GETH,
    NODE_TYPE_PARITY
} BREthereumLESNodeType;

/**
 * A Node will callback on: state changes, announcements (of block), and results.
 * The callback includes a User context.
 */
typedef void *BREthereumLESNodeContext;

typedef void
(*BREthereumNodeCallbackStatus) (BREthereumLESNodeContext context,
                                 BREthereumLESNode node,
                                 BREthereumHash headHash,
                                 uint64_t headNumber);

typedef void
(*BREthereumNodeCallbackAnnounce) (BREthereumLESNodeContext context,
                                   BREthereumLESNode node,
                                   BREthereumHash headHash,
                                   uint64_t headNumber,
                                   UInt256 headTotalDifficulty,
                                   uint64_t reorgDepth);

/**
 * A Node defines a `Provide Callback` type to include the context, the node, and the result.
 */
typedef void
(*BREthereumLESNodeCallbackProvide) (BREthereumLESNodeContext context,
                                     BREthereumLESNode node,
                                     BREthereumProvisionResult result);


typedef void
(*BREthereumLESNodeCallbackMessage) (BREthereumLESNodeContext context,
                                     BREthereumLESNode node,
                                     BREthereumLESMessage message);

typedef void
(*BREthereumLESNodeCallbackNeighbor) (BREthereumLESNodeContext context,
                                      BREthereumLESNode node,
                                      BREthereumDISNeighbor neighbor);

/// MARK: LES Node State

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
} BREthereumLESNodeProtocolReason;

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
            BREthereumLESNodeProtocolReason reason;
        } protocol;
    } u;
} BREthereumLESNodeState;

/** Fills description, returns description */
extern const char *
nodeStateDescribe (const BREthereumLESNodeState *state,
                   char description[128]);

extern BRRlpItem
nodeStateEncode (const BREthereumLESNodeState *state,
                 BRRlpCoder coder);

extern BREthereumLESNodeState
nodeStateDecode (BRRlpItem item,
                 BRRlpCoder coder);

typedef void
(*BREthereumLESNodeCallbackState) (BREthereumLESNodeContext context,
                                   BREthereumLESNode node,
                                   BREthereumLESNodeEndpointRoute route,
                                   BREthereumLESNodeState state);

// connect
// disconnect
// network reachable

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
            BREthereumNodeCallbackStatus callbackStatus,
            BREthereumNodeCallbackAnnounce callbackAnnounce,
            BREthereumLESNodeCallbackProvide callbackProvide,
            BREthereumLESNodeCallbackNeighbor callbackNeighbor,
            BREthereumLESNodeCallbackState callbackState,
            BREthereumLESNodeCallbackMessage callbackMessage);

extern void
nodeRelease (BREthereumLESNode node);

extern void
nodeConnect (BREthereumLESNode node,
             BREthereumLESNodeEndpointRoute route);

extern void
nodeDisconnect (BREthereumLESNode node,
                BREthereumLESNodeEndpointRoute route,
                BREthereumP2PDisconnectReason reason);

extern int
nodeUpdateDescriptors (BREthereumLESNode node,
                       BREthereumLESNodeEndpointRoute route,
                       fd_set *recv,   // read
                       fd_set *send);  // write

extern void
nodeProcessDescriptors (BREthereumLESNode node,
                        BREthereumLESNodeEndpointRoute route,
                        fd_set *recv,   // read
                        fd_set *send);  // write

extern void
nodeHandleProvision (BREthereumLESNode node,
                       BREthereumProvision provision);

extern BREthereumLESNodeEndpoint *
nodeGetRemoteEndpoint (BREthereumLESNode node);

extern BREthereumLESNodeEndpoint *
nodeGetLocalEndpoint (BREthereumLESNode node);

extern int
nodeHasState (BREthereumLESNode node,
              BREthereumLESNodeEndpointRoute route,
              BREthereumLESNodeStateType type);

extern BREthereumLESNodeState
nodeGetState (BREthereumLESNode node,
              BREthereumLESNodeEndpointRoute route);

extern void
nodeSetStateInitial (BREthereumLESNode node,
                     BREthereumLESNodeEndpointRoute route,
                     BREthereumLESNodeState state);

extern BREthereumBoolean
nodeGetDiscovered (BREthereumLESNode node);

extern void
nodeSetDiscovered (BREthereumLESNode node,
                   BREthereumBoolean discovered);

extern void
nodeDiscover (BREthereumLESNode node,
              BREthereumLESNodeEndpoint *endpoint);
    
extern size_t
nodeHashValue (const void *node);

extern int
nodeHashEqual (const void *node1,
               const void *node2);

/// MARK: Node Message Send/Recv
    
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

extern void
nodeShow (BREthereumLESNode node);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Node_H */
