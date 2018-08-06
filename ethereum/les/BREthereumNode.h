//
//  BREthereumNode.h 
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/16/18.
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

#ifndef BR_Ethereum_Node_h
#define BR_Ethereum_Node_h

#include <stddef.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "BRInt.h"
#include "../base/BREthereumBase.h"
#include "BREthereumAccount.h"
#include "BREthereumNodeDiscovery.h"
#include "BRKey.h"
#include "BREthereumLES.h"
#include "BREthereumP2PCoder.h"
#include "BREthereumFrameCoder.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum LES Node
 *
 */
typedef struct BREthereumLESNodeContext* BREthereumLESNode;

/**
 * Ehtereum Node Connection Status - Connection states for a node
 */
typedef enum {
    BRE_NODE_ERROR = -1, 
    BRE_NODE_DISCONNECTED,
    BRE_NODE_DICONNECTING, 
    BRE_NODE_CONNECTING,
    BRE_NODE_PERFORMING_HANDSHAKE,
    BRE_NODE_CONNECTED
} BREthereumNodeStatus;

/**
 * BREthereumPeerConfig - the initial data needed to begin a connection to a remote node
 */
typedef struct {
    BREthereumLESEndpoint endpoint; //endpoint information for the remote
    uint64_t timestamp; // the timestamp of peer configuration
    BRKey* remoteKey;   // the remote public key for the remote peer
} BREthereumLESPeerConfig;


typedef void* BREthereumManagerCallbackContext;
typedef void (*BREthereumManagerDisconnectCallback)(BREthereumManagerCallbackContext info, BREthereumLESNode node, BREthereumLESDisconnect reason);
typedef void (*BREthereumManagerRecMsgCallback)(BREthereumManagerCallbackContext info, BREthereumLESNode node, uint64_t packetType, BRRlpData messageBody);
typedef void (*BREthereumManagerConnectedCallback)(BREthereumManagerCallbackContext info, BREthereumLESNode node, uint8_t** status, size_t* statusSize);
typedef void (*BREthereumManagerNetworkReachableCallback)(BREthereumManagerCallbackContext info, BREthereumLESNode node, BREthereumBoolean isReachable);

typedef struct {
    BREthereumManagerCallbackContext info;
    BREthereumManagerDisconnectCallback disconnectFunc;
    BREthereumManagerRecMsgCallback receivedMsgFunc;
    BREthereumManagerConnectedCallback connectedFuc;
    BREthereumManagerNetworkReachableCallback networkReachableFunc;
}BREthereumManagerCallback;

/**
 * Creates an ethereum node with the remote peer information and whether the node should send
 * an auth message first.
 *
 *
 *
 */ 
extern BREthereumLESNode nodeCreate(BREthereumLESPeerConfig config,
                                         BRKey* key,
                                         UInt256* nonce,
                                         BRKey* ephemeral,
                                         BREthereumManagerCallback callbacks,
                                         BREthereumBoolean originate);

/**
 * Deletes the memory of the ethereum node
 * @param node - the node context
 */
extern void nodeRelease(BREthereumLESNode node);


/**
 * Retrieves the status of an ethereum node
 * @param node - the node context
 */
extern BREthereumNodeStatus nodeStatus(BREthereumLESNode node);

/**
 * Connects to the ethereum node to a remote node
 * @param node - the node context
 */
extern int nodeConnect(BREthereumLESNode node);

/**
 * Disconnects the ethereum node from a remote node
 * @param node - the node context
 */
extern void nodeDisconnect(BREthereumLESNode node, BREthereumLESDisconnect reason);

/**
 * Sends a packet (i.e. a frame) to the remote node
 * @param node - the node context
 * @param packetType - the id of the packet being sent to the remote ndoe
 * @param payload - the payload to send to the remote node
 */
extern BREthereumBoolean nodeSendMessage(BREthereumLESNode node, uint64_t packetType, uint8_t* payload, size_t payloadSize);

/**
 * Determines if two nodes are the same
 * @param node1 - the first node to compare
 * @param node2 - the second ndoe to compare
 * @param True, if they are same, otherwise False
 */
extern BREthereumBoolean nodeEQ(BREthereumLESNode node1, BREthereumLESNode node2); 

/**
 * Retrieves the key for this node
 * @param node - the node context
 */
 extern BRKey* nodeGetKey(BREthereumLESNode node);
 
/**
 * Retrieves the remote key that this node is connected to
 * @param node - the node context
 */
 extern BRKey* nodeGetPeerKey(BREthereumLESNode node);

/**
 * Retrieves the frame coder for this node
 * @param node - the node context
 * @return the frame coder associated with the node
 */
extern BREthereumLESFrameCoder nodeGetFrameCoder(BREthereumLESNode node); 

/**
 * Retrieves the RLP encoded status message for this node
 * @param node - the node context
 * @return the rlp data of the status message for this node
 */
extern BRRlpData nodeGetStatusData(BREthereumLESNode node);

/**
 * Determines whether this node is originating the connection
 * @param node - the node context
 * @return True, if the local node (the parameter "node") originated the handshake connection, otherwise false
 */
extern BREthereumBoolean nodeDidOriginate(BREthereumLESNode node);

/**
 * Retrieves a reference to the local ephemeral
 * @param node - the node context
 * @return the local emphermeral key
 */
extern BRKey* nodeGetEphemeral(BREthereumLESNode node);

/**
 * Retrieves a reference to the remote ephemeral
 * @param node - the node context
 * @return the remote empheremral key
 */
extern BRKey* nodeGetPeerEphemeral(BREthereumLESNode node);

/**
 * Retrieves a reference to the local nonce
 * @param node - the node context
 * @param the local nonce
 */
extern UInt256* nodeGetNonce(BREthereumLESNode node);

/**
 * Retrieves a reference to the remote nonce
 * @param node - the node context
 * @return the remote node's nonce
 */
extern UInt256* nodeGetPeerNonce(BREthereumLESNode node);

/**
 * Retrieve the Hello Message for the node
 * @param node - the node context
 * @return the rlp data of the p2p hello message for this node
 */
extern BRRlpData nodeRLPP2PHello(BREthereumLESNode node);

/**
 * Reads information from the peer
 * @param node - the node context
 * @param buff - the buffer to send to the remote peer
 * @param bufSize - the size of the buffer to send
 */
extern int nodeReadFromPeer(BREthereumLESNode node, uint8_t * buf, size_t bufSize, const char * type);

/**
 * Writes information to the peer
* @param node - the node context
 * @param buff - the buffer to read from the remote peer
 * @param bufSize - the size of the buffer read 
 */
extern int nodeWriteToPeer(BREthereumLESNode node, uint8_t * buf, size_t bufSize, char* type);


#ifdef __cplusplus
}
#endif

#endif // BR_Ethereum_Node_h
