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
#include "BREthereumBase.h"
#include "BREthereumAccount.h"
#include "BREthereumNodeDiscovery.h"
#include "BREthereumNodeEventHandler.h"
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
typedef struct BREthereumNodeContext* BREthereumNode;

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
    BREthereumEndpoint endpoint; //endpoint information for the remote
    uint64_t timestamp; // the timestamp of peer configuration
    BRKey* remoteKey;   // the remote public key for the remote peer
} BREthereumPeerConfig;


typedef void* BREthereumManagerCallbackContext;
typedef void (*BREthereumManagerDisconnectCallback)(BREthereumManagerCallbackContext info, BREthereumNode node, BREthereumDisconnect reason);
typedef void (*BREthereumManagerRecMsgCallback)(BREthereumManagerCallbackContext info, BREthereumNode node, uint64_t packetType, BRRlpData messageBody);
typedef void (*BREthereumManagerConnectedCallback)(BREthereumManagerCallbackContext info, BREthereumNode node, uint8_t** status, size_t* statusSize);
typedef void (*BREthereumManagerNetworkReachableCallback)(BREthereumManagerCallbackContext info, BREthereumNode node, BREthereumBoolean isReachable);

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
 */ 
extern BREthereumNode ethereumNodeCreate(BREthereumPeerConfig config,
                                         BRKey* key,
                                         UInt256* nonce,
                                         BRKey* ephemeral,
                                         BREthereumManagerCallback callbacks,
                                         BREthereumBoolean originate);

/**
 * Deletes the memory of the ethereum node
 */
extern void ethereumNodeRelease(BREthereumNode node);


/**
 * Retrieves the status of an ethereum node
 */
extern BREthereumNodeStatus ethereumNodeStatus(BREthereumNode node);

/**
 * Connects to the ethereum node to a remote node
 */
extern int ethereumNodeConnect(BREthereumNode node);

/**
 * Disconnects the ethereum node from a remote node
 */
extern void ethereumNodeDisconnect(BREthereumNode node, BREthereumDisconnect reason);

/**
 * Sends a message to the remote node
 */
extern BREthereumBoolean ethereumNodeSendMessage(BREthereumNode node, uint64_t packetType, uint8_t* payload, size_t payloadSize);

/**
 * Determines if the given BREthereumNode is the same as t
 */
extern BREthereumBoolean ethereumNodeEQ(BREthereumNode node1, BREthereumNode node2); 

/**
 * Retrieves the key for this node
 */
 extern BRKey* ethereumNodeGetKey(BREthereumNode node);
 
/**
 * Retrieves the remote key that this node is connected to
 */
 extern BRKey* ethereumNodeGetPeerKey(BREthereumNode node);

/**
 * Retrieves the frame coder for this node
 */
extern BREthereumFrameCoder ethereumNodeGetFrameCoder(BREthereumNode node); 

/**
 * Retrieves the RLP encoded status message for this node
 */
extern BRRlpData ethereumNodeGetStatusData(BREthereumNode node);

/**
 * Determines whether this node is originating the connection
 */
extern BREthereumBoolean ethereumNodeDidOriginate(BREthereumNode node);

/**
 * Retrieves a reference to the local ephemeral
 */
extern BRKey* ethereumNodeGetEphemeral(BREthereumNode node);

/**
 * Retrieves a reference to the remote ephemeral
 */
extern BRKey* ethereumNodeGetPeerEphemeral(BREthereumNode node);

/**
 * Retrieves a reference to the local nonce
 */
extern UInt256* ethereumNodeGetNonce(BREthereumNode node);

/**
 * Retrieves a reference to the remote nonce
 */
extern UInt256* ethereumNodeGetPeerNonce(BREthereumNode node);

/**
 * Retrieve the Hello Message for the node
 */
extern BRRlpData ethereumNodeRLPP2PHello(BREthereumNode node);

/**
 * Decode the Hello Message from the peer
 */
extern BREthereumBoolean ethereumNodePeerP2PHello(BREthereumNode node);

/**
 * Announces an event to the node
 */
extern void ethereumNodeAnnounceEvent(BREthereumNode node, BREthereumNodeEvent evt);

/**
 * Reads information from the peer
 */
extern int ethereumNodeReadFromPeer(BREthereumNode node, uint8_t * buf, size_t bufSize, const char * type);

/**
 * Writes information to the peer
 */
extern int ethereumNodeWriteToPeer(BREthereumNode node, uint8_t * buf, size_t bufSize, char* type);


#ifdef __cplusplus
}
#endif

#endif // BR_Ethereum_Node_h
