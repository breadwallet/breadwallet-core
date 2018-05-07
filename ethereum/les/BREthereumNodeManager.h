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
#include "BREthereumNetwork.h"
#include "BREthereumTransaction.h"
#include <inttypes.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumNodeMangerContext* BREthereumNodeManager;

/**
 * Ehtereum Manager Node Connection Status - Connection states for Manager
 */
typedef enum {
    BRE_MANAGER_ERROR = -1,
    BRE_MANAGER_DISCONNECTED,  //Manager is not connected to any remote node(s)
    BRE_MANAGER_CONNECTING,    //Manager is trying to connect to remote node(s)
    BRE_MANAGER_CONNECTED      //Manager is connected to a remote node(s) 
} BREthereumNodeManagerStatus;

//
// Ethereum Node Manager management functions
//

/**
 * Creates a new Ethereum Node manager.
 * @post: Must be released by a calling ethereumNodeManagerRelease(manager)
 * @param network the Ethereum network to connect to remote peers
 */
extern BREthereumNodeManager ethereumNodeManagerCreate(BREthereumNetwork network);

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
extern void ethereumNodeMangerConnect(BREthereumNodeManager manager);

/**
 * Disconnects from the ethereum peer-to-peer network.
 * @param manager - the node manager context
 */
extern void ethereumNodeManagerDisconnect(BREthereumNodeManager manager);
 
/**
 * Determines the number of remote peers that are successfully connected to the manager
 * @param manager - the node manager context
 * @return the number of connected remote peers
 */
extern size_t ethereumNodeMangerPeerCount(BREthereumNodeManager manager);

//
// Ethereum Node Manager LES types and functions
//

//Callback definitions for the LES functions
typedef void (*BRLESGetTransactions)(BREthereumTransaction* transactions,
                                     size_t transactionsSize,
                                     unsigned int requestId);
    
    
/**
 * Requets a remote peer to submit a transaction into its transaction pool and relay them to the ETH network.
 * @param transaction - the tranaction to submit
 * @param requestId - a unique id for this transaction submission.
 * @return ETHEREUM_BOOLEAN_TRUE, if the transaction was successfully submited to the peer. Otherwise, ETHEREUM_BOOLEAN_FALSE is returned on error.
 */
extern BREthereumBoolean ethereumNodeManagerSubmitTransaction(BREthereumNodeManager manager,
                                                              const BREthereumTransaction transaction,
                                                              const int requestId);


/**
 * Requets a remote peer to retrieve transactions from the remote-peer
 * @param address - the primary wallet address
 * @param a unique id for this transaction request.
 * @param callback - the callback function which will be caleld once the transactions are received from the network
 */
extern void ethereumNodeManagerGetTransaction(BREthereumNodeManager manager,
                                              const char *address,
                                              const int requestId,
                                              BRLESGetTransactions callback);


#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_NodeManager_h */
