//
//  BREthereumNodeManager.c
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

#include <pthread.h>
#include <errno.h>
#include "BREthereumNodeManager.h"
#include "BREthereumNode.h"
#include "BRArray.h"

typedef struct {

    //The array of ethereum node
    BREthereumNode* nodes;
    
    //The array of pointers that point to the nodes that
    // are connected in the "nodes" field
    BREthereumNode** connectedNodes;

   // A lock for the manager context
   pthread_mutex_t lock;
   
}BREthereumNodeManagerContext;

#define ETHEREUMN_PEER_MAX_CONNECTIONS 5

BREthereumNodeManager createEthereumNodeManager(void) {

    BREthereumNodeManagerContext* manager= (BREthereumNodeManagerContext*) calloc(1, sizeof (BREthereumNodeManagerContext));
    
    array_new(manager->nodes, ETHEREUMN_PEER_MAX_CONNECTIONS);
    array_new(manager->connectedNodes, ETHEREUMN_PEER_MAX_CONNECTIONS);
    
    pthread_mutex_init(&manager->lock, NULL);
    
    return (BREthereumNodeManager)manager;
}
void freeEthereumNodeManager(BREthereumNodeManager manager) {
    
    BREthereumNodeManagerContext* manager= (BREthereumNodeManagerContext*) manager;
    array_free(manager->nodes);
    array_free(manager->connectedNodes);
    free(manager);
}
BREthereumLESNodeStatus ethereumNodeManagerStatus(BREthereumNodeManager manager){
 
    return BRE_LESNODE_DISCONNECTED;
    
}
void connectEthereumNodeManager(BREthereumNodeManager manager) {

    //TODO: Implement the connection to remote ethereum nodes
    // 1. Discover Nodes to connect to
    // 2. Create BREthernumNodes to connect to discovered nodes

}
void disconnectEthereumNodeManager(BREthereumNodeManager manager) {

    //TODO: Implement a disconnection to remote ethereum nodes
    // 1.

}
size_t peerCountForEthereumNodeManager(BREthereumNodeManager manager) {

    BREthereumNodeManagerContext* manager= (BREthereumNodeManagerContext*) manager;
    return (size-t) array_count(manager->connected);
}
