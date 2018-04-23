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

#include  "BREthereumNode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumNodeMangerContext* BREthereumNodeManager;

/**
 * Creates a new Ethereum Node manager.
 * @postcondition: Must be freed by a calling freeEthereumNodeManager()
 */
BREthereumNodeManager createEthereumNodeManager(void);

/**
 * Frees the memory assoicated with the given node manager.
 * @param manager - the node manager to free
 */
void freeEthereumNodeManager(BREthereumNodeManager manager);

/**
 * Determines whether one of the nodes is connected to a remote node
 */
 BREthereumNodeStatus ethereumNodeManagerStatus(BREthereumNodeManager manager);
 
 /**
  * Connects to the ethereum peer-to-peer network.
  * @param manager - the node manager context
  */
void connectEthereumNodeManager(BREthereumNodeManager manager);

/**
 * Disconnects from the ethereum peer-to-peer network.
 * @param manager - the node manager context
 */
 void disconnectEthereumNodeManager(BREthereumNodeManager manager);
 
/**
 * Returns the number of remote peers the nodes are connected to
 * @param manager - the node manager context
 */
 size_t peerCountForEthereumNodeManager(BREthereumNodeManager manager);
 
 
#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_NodeManager_h */
