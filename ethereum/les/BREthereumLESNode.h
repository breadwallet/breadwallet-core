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
#include "BREthereumLESProvision.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/// MARK: LES Node Provision

/**
 * A Node identies each request with an identifier.  (This is a
 * message reqId too....)
 */
typedef uint64_t BREthereumNodeProvisionIdentifier;

/**
 * A Node provides its results in a self-identifying-type union of the request types.
 */
typedef struct {
    /** The provision identifier; match with the reqeust */
    BREthereumNodeProvisionIdentifier identifier;

    /** The provision as a union of {reqeust, response} for each provision type. */
    BREthereumNodeProvision provision;

    /**
     * The limit for each message.  When constructing the 'response' from a set of messages we'll
     * expect eash message to have this many individual responses (except for the last message
     * which may have fewer).
     */
    size_t messageContentLimit;

    /** Teh count of messages */
    size_t messagesCount;

    /** The count of messages remaining */
    size_t messagesRemainingCount;

    /** Time of creation */
    long timestamp;

} BREthereumLESNodeProvisionPending;

/**
 * A Node defines a `Provide Callback` type to include the context, the node, and the result.
 */
typedef void
(*BREthereumLESNodeCallbackProvide) (BREthereumLESNodeContext context,
                                     BREthereumLESNode node,
                                     BREthereumNodeProvisionResult result);


/// MARK: LES Node Message (Callback)

typedef void
(*BREthereumLESNodeCallbackMessage) (BREthereumLESNodeContext context,
                                     BREthereumLESNode node,
                                     BREthereumLESMessage message);


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

/**
 * Provide Block Headers
 *
 * @abstract Produces a result with type NODE_PROVIDE_BLOCK_HEADERS
 *
 * @param node The node
 * @param start The block number of the first header
 * @param skip The block numbers to skip between headers, normally '0'
 * @param limit The number of headers to provide
 * @param reverse TRUE if the headers should be provided in reverse order
 *
 * @return the provide identifier
 */
extern BREthereumNodeProvisionIdentifier
nodeProvideBlockHeaders (BREthereumLESNode node,
                         uint64_t start,
                         uint64_t skip,
                         uint32_t limit,
                         BREthereumBoolean reverse);


/**
 * Provide Block Bodies
 *
 * @abstract Produces a result with type NODE_PROVIDE_BLOCK_BODIES
 *
 * @param node The node
 * @param BREthereumHash An array of hashes
 *
 * @return the provide identifier
 */
extern BREthereumNodeProvisionIdentifier
nodeProvideBlockBodies (BREthereumLESNode node,
                        BRArrayOf(BREthereumHash) headerHashes);


/**
 * Provide Transaction Receipts
 *
 * @abstract Produces a result with type NODE_PROVIDE_TRANSACTION_RECEIPTS
 *
 * @param node The node
 * @param BREthereumHash An array of hashes
 *
 * @return the provide identifier
 */
extern BREthereumNodeProvisionIdentifier
nodeProvideTransactionReceipts (BREthereumLESNode node,
                                BRArrayOf(BREthereumHash) headerHashes);


/**
 *
 *
 * @abstract Produces a result with type NODE_PROVIDE_ACCOUNTS
 *
 * @param node The Node
 * @param address The account's address
 * @param BREthereumHash An array of hashes
 *
 * @return the provide identifier
 */
extern BREthereumNodeProvisionIdentifier
nodeProvideAccounts (BREthereumLESNode node,
                     BREthereumAddress address,
                     BRArrayOf(BREthereumHash) headerHashes);

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
            BREthereumLESNodeCallbackMessage callbackMessage,
            BREthereumLESNodeCallbackState callbackStatus,
            BREthereumLESNodeCallbackProvide callbackProvide);

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
                           BREthereumLESNodeProtocolReason reason);

extern void
nodeSetStateInitial (BREthereumLESNode node,
                     BREthereumLESNodeEndpointRoute route,
                     BREthereumLESNodeState state);

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
