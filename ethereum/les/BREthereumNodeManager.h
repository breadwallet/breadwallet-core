//
//  BREthereumNodeManager.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/19/18.
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

#ifndef BR_Ethereum_NodeManager_h
#define BR_Ethereum_NodeManager_h

#include "BREthereumNode.h"
#include "../blockchain/BREthereumBlockChain.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumNodeManagerContext* BREthereumNodeManager;


/**
 * Ehtereum Manager Node Connection Status - Connection states for Manager
 */
typedef enum {
    BRE_MANAGER_DISCONNECTED =0,  //Manager is not connected to any remote node(s)
    BRE_MANAGER_CONNECTING,    //Manager is trying to connect to remote node(s)
    BRE_MANAGER_CONNECTED      //Manager is connected to a remote node(s) 
} BREthereumNodeManagerStatus;

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

/**
 * Creates a new Ethereum Node manager.
 * @post: Must be released by a calling ethereumNodeManagerRelease(manager)
 * @param key - the ethereum private key that is needed to assign nodes their own public keys
 * @param headTd - Total Difficulty of the best chain. Integer, as found in block header.
 * @param headHash - the hash of the best (i.e. highest TD) known block.
 * @param headNum - the number of the best (i.e. highest TD) known block.
 * @param genesisHash - the hash of the Genesis block.
 * @param callbacks - the context that contains the callbacks that are used when the manager receives messags from a remote node
 */
extern BREthereumNodeManager
ethereumNodeManagerCreate(BREthereumNetwork network,
                          BRKey* key,
                          BREthereumHash headHash,
                          uint64_t headNumber,
                          UInt256 headTotalDifficulty,
                          BREthereumHash genesisHash,
                          BREthereumSubProtoCallbacks callbacks);
    
/**
 * Frees the memory assoicated with the given node manager.
 * @param manager - the node manager to release
 */
extern void ethereumNodeManagerRelease(BREthereumNodeManager manager);

/**
 * Determines whether one of the nodes is connected to a remote node
 * @param manager - the node manager
 * @return  the status of the node manager
 */
extern BREthereumNodeManagerStatus ethereumNodeManagerStatus(BREthereumNodeManager manager);
 
 /**
  * Connects to the ethereum peer-to-peer network.
  * @param manager - the node manager context
  */
extern int ethereumNodeMangerConnect(BREthereumNodeManager manager);

/**
 * Disconnects from the ethereum peer-to-peer network.
 * @param manager - the node manager context
 */
extern void ethereumNodeManagerDisconnect(BREthereumNodeManager manager);

/**
 * Sends a message to a remote node
 */
extern BREthereumBoolean ethereumNodeManagerSendMessage(BREthereumNodeManager manager, uint64_t packetType, uint8_t* payload, size_t payloadSize);
 

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_NodeManager_h */
