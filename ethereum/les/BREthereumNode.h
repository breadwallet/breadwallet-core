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
#include "BREthereumNodeEventHandler.h"
#include "BRKey.h"
#include "BREthereumLES.h"
#include "BREthereumFrameCoder.h"
// Note:: Duplicated this logging code from Aaron's BRPeer.h file
// TODO: May want to move this code into it's own library
#define bre_node_log(node, ...) _bre_node_log("%s:%"PRIu16" " _va_first(__VA_ARGS__, NULL) "\n", ethereumNodeGetPeerHost(node),\
ethereumNodeGetPeerPort(node), _va_rest(__VA_ARGS__, NULL))
#define _va_first(first, ...) first
#define _va_rest(first, ...) __VA_ARGS__

#if defined(TARGET_OS_MAC)
#include <Foundation/Foundation.h>
#include <stdio.h> 
#define _bre_node_log(...) printf(__VA_ARGS__)
#elif defined(__ANDROID__)
#include <android/log.h>
#define _bre_node_log(...) __android_log_print(ANDROID_LOG_INFO, "bread", __VA_ARGS__)
#else
#include <stdio.h>
#define _bre_node_log(...) printf(__VA_ARGS__)
#endif

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
    BRE_NODE_CONNECTING,
    BRE_NODE_PERFORMING_HANDSHAKE,
    BRE_NODE_CONNECTED
} BREthereumNodeStatus;

/**
 * BREthereumPeerConfig - the initial data needed to begin a connection to a remote node
 */
typedef struct {
    UInt128 address;    // IPv6 address of peer
    uint16_t port;      // port number for peer connection
    uint64_t timestamp; // timestamp reported by peer
} BREthereumPeerConfig;

/**
 * Creates an ethereum node with the remote peer information and whether the node should send
 * an auth message first. 
 */ 
extern BREthereumNode ethereumNodeCreate(BREthereumPeerConfig config,
                                         BREthereumBoolean originate);

/**
 * Retrieves the status of an ethereum node
 */
extern BREthereumNodeStatus ethereumNodeStatus(BREthereumNode node);

/**
 * Connects to the ethereum node to a remote node
 */
extern void ethereumNodeConnect(BREthereumNode node);

/**
 * Disconnects the ethereum node from a remote node
 */
extern void ethereumNodeDisconnect(BREthereumNode node);

/**
 * Retrieves the key for this node
 */
 extern BRKey* ethereumNodeGetKey(BREthereumNode node);
 
/**
 * Retrieves the remote key that this node is connected to
 */
 extern BRKey* ethereumNodeGetRemoteKey(BREthereumNode node);

/**
 * Retrieves the frame coder for this node
 */
extern BREthereumFrameCoder ethereumNodeGetFrameCoder(BREthereumNode node); 

/**
 * Deletes the memory of the ethereum node
 */
extern void ethereumNodeFree(BREthereumNode node);

/**
 * Retrieves the RLP encoded status message for this node
 */
extern BRRlpData ethereumNodeGetStatusData(BREthereumNode node);

/**
 * Determines whether this node is originating the connection
 */
extern BREthereumBoolean ethereumNodeDidOriginate(BREthereumNode node);

/**
 * Retrives a reference to the remote status for the remote peer
 */
extern BREthereumLESStatus* ethereumNodeGetPeerStatus(BREthereumNode node);

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
extern BRRlpData ethereumNodeGetEncodedHelloData(BREthereumNode node);

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

/**
 * Retrieves the string representation of the remot peer address
 */ 
extern const char * ethereumNodeGetPeerHost(BREthereumNode node);

/**
 * Retrieves the port of the remot peer address
 */
extern uint16_t ethereumNodeGetPeerPort(BREthereumNode node);

#ifdef __cplusplus
}
#endif

#endif // BR_Ethereum_Node_h
