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
#include "../blockchain/BREthereumNetwork.h"
#include "BREthereumEndpoint.h"
#include "BREthereumRandom.h"
#include "../util/BRUtil.h"

struct BREthereumNodeManagerContext {

    //The network chain for this manager
    BREthereumNetwork network;
    
    //The hash for the head block
    BREthereumHash headHash;
    
    //The head number for the head block
    uint64_t headNumber;
    
    //The head total diffculty
    UInt256 headTotalDifficulty;
    
    //The head  has for the genesis node
    BREthereumHash genesisHash;
    
    //The callbacks for the subprotocool
    BREthereumSubProtoCallbacks  subprotoCallbacks;
    
    //The callback functions that are passed to nodes
    BREthereumManagerCallback managerCallbacks;
    
    //The remote peers that the manager can connect to
    BREthereumPeerConfig* peers;
    
    //The currently connected nodes
    BREthereumNode* connectedNodes;

    //The random context for generating random data
    BREthereumRandomContext randomContext;

    // A lock for the manager context
    pthread_mutex_t lock;
};

#define PEERS_THRESHOLD 100
#define ETHEREUM_PEER_MAX_CONNECTIONS 5
#define BOOTSTRAP_NODE_IDX 0
#define ETH_LOG_TOPIC "BREthereumManager"
#define DEFAULT_TCPPORT 30303
#define DEFAULT_UDPPORT 30303

//
// Private Functions & types
BREthereumEndpoint _brd_bootstrap_peer;

void _disconnectCallback(BREthereumManagerCallbackContext info, BREthereumNode node, BREthereumDisconnect reason) {

    BREthereumNodeManager manager = (BREthereumNodeManager)info;
    pthread_mutex_lock(&manager->lock);
    for(int i = 0; i < array_count(manager->connectedNodes); ++i) {
    
        if(ETHEREUM_BOOLEAN_IS_TRUE(ethereumNodeEQ(node, manager->connectedNodes[i]))){
            array_rm(manager->connectedNodes, i);
            break;
        }
    }
    pthread_mutex_unlock(&manager->lock);
}
void _receivedMessageCallback(BREthereumManagerCallbackContext info, BREthereumNode node, uint64_t packetType, BRRlpData messageBody) {

    BREthereumNodeManager manager = (BREthereumNodeManager)info;
    if (NULL != manager->subprotoCallbacks.messageRecFunc)
        manager->subprotoCallbacks.messageRecFunc(manager->subprotoCallbacks.info, packetType, messageBody);
}
void _connectedCallback(BREthereumManagerCallbackContext info, BREthereumNode node, uint8_t**status, size_t* statusSize) {

    BREthereumNodeManager manager = (BREthereumNodeManager)info;
    if (NULL != manager->subprotoCallbacks.connectedFunc)
        manager->subprotoCallbacks.connectedFunc(manager->subprotoCallbacks.info, status,statusSize);
}
void _networkReachableCallback(BREthereumManagerCallbackContext info, BREthereumNode node, BREthereumBoolean isReachable) {

    BREthereumNodeManager manager = (BREthereumNodeManager)info;
    manager->subprotoCallbacks.networkReachFunc(manager->subprotoCallbacks.info, isReachable);
}
BREthereumBoolean _findPeers(BREthereumNodeManager manager) {

    //Note: This function should be called from within the lock of the les context
    //For testing purposes, we will only connect to our remote node known
    BREthereumBoolean ret = ETHEREUM_BOOLEAN_FALSE;

    if(array_count(manager->connectedNodes) == 0)
    {
        BREthereumPeerConfig config;
        uint8_t pubKey[64];

#if 0 // 65...
        config.endpoint = ethereumEndpointCreate(ETHEREUM_BOOLEAN_TRUE, "65.79.142.182", 30303, 30303);
        decodeHex (pubKey, 64, "c7f12332d767c12888da45044581d30de5a1bf383f68ef7b79c83eefd99c82adf2ebe3f37e472cbcdf839d52eddc34f270a7a3444ab6c1dd127bba1687140d93", 128);
#elif 1 // 104...
        config.endpoint = ethereumEndpointCreate(ETHEREUM_BOOLEAN_TRUE, "104.197.99.24", DEFAULT_TCPPORT, DEFAULT_UDPPORT);
        decodeHex (pubKey, 64, "e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851", 128);
#elif 0 // TestNet: 35...198
        config.endpoint = ethereumEndpointCreate(ETHEREUM_BOOLEAN_TRUE, "35.226.161.198", 30303, 30303); //TestNet
        decodeHex (pubKey, 64, "9683a29b13c7190cfd63cccba4bcb62d7b710da0b1c4bff2c4b8bcf129127d7b3f591163a58449d7f66200db3c208d06b9e9a8bea69be4a72e1728a83d703063", 128);
#elif 0   // 35...
        config.endpoint = ethereumEndpointCreate(ETHEREUM_BOOLEAN_TRUE, "35.184.255.33", 30303, 30303);
        decodeHex (pubKey, 64, "3d0bce4775635c65733b7534f1bccd48720632f5d66a44030c1d13e2e5883262d9d22cdb8365c03137e8d5fbbf5355772acf35b08d6f9b5ad69bb24ad52a20cc", 128);
#elif 0  // public
        // admin.addPeer("enode://3e9301c797f3863d7d0f29eec9a416f13956bd3a14eec7e0cf5eb56942841526269209edf6f57cd1315bef60c4ebbe3476bc5457bed4e479cac844c8c9e375d3@109.232.77.21:30303");
        config.endpoint = ethereumEndpointCreate(ETHEREUM_BOOLEAN_TRUE, "109.232.77.21", 30303, 30303);
        decodeHex (pubKey, 64, "3e9301c797f3863d7d0f29eec9a416f13956bd3a14eec7e0cf5eb56942841526269209edf6f57cd1315bef60c4ebbe3476bc5457bed4e479cac844c8c9e375d3", 128);
#elif 0  // public
        //admin.addPeer("enode://e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851@35.226.238.26:30303");
        config.endpoint = ethereumEndpointCreate(ETHEREUM_BOOLEAN_TRUE, "35.226.238.26", 30303, 30303);
        decodeHex (pubKey, 64, "e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851", 128);
#elif 0 // parity - maybe these should work, but don't
        // enode://81863f47e9bd652585d3f78b4b2ee07b93dad603fd9bc3c293e1244250725998adc88da0cef48f1de89b15ab92b15db8f43dc2b6fb8fbd86a6f217a1dd886701@193.70.55.37:30303
        // enode://4afb3a9137a88267c02651052cf6fb217931b8c78ee058bb86643542a4e2e0a8d24d47d871654e1b78a276c363f3c1bc89254a973b00adc359c9e9a48f140686@144.217.139.5:30303
        // enode://c16d390b32e6eb1c312849fe12601412313165df1a705757d671296f1ac8783c5cff09eab0118ac1f981d7148c85072f0f26407e5c68598f3ad49209fade404d@139.99.51.203:30303
        // enode://029178d6d6f9f8026fc0bc17d5d1401aac76ec9d86633bba2320b5eed7b312980c0a210b74b20c4f9a8b0b2bf884b111fa9ea5c5f916bb9bbc0e0c8640a0f56c@216.158.85.185:30303
        // enode://fdd1b9bb613cfbc200bba17ce199a9490edc752a833f88d4134bf52bb0d858aa5524cb3ec9366c7a4ef4637754b8b15b5dc913e4ed9fdb6022f7512d7b63f181@212.47.247.103:30303

        config.endpoint = ethereumEndpointCreate(ETHEREUM_BOOLEAN_TRUE, "193.70.55.37", 30303, 30303);
        decodeHex (pubKey, 64, "81863f47e9bd652585d3f78b4b2ee07b93dad603fd9bc3c293e1244250725998adc88da0cef48f1de89b15ab92b15db8f43dc2b6fb8fbd86a6f217a1dd886701", 128);
#elif 0 // Ed's Local Parity
        // enode://8ebe6a85d46737451c8bd9423f37dcb117af7316bbce1643856feeaf9f81a792ff09029e9ab1796b193eb477f938af3465f911574c57161326b71aaf0221f341@192.168.1.105:30303
        config.endpoint = ethereumEndpointCreate(ETHEREUM_BOOLEAN_TRUE, "192.168.1.105", 30303, 30303);
        decodeHex (pubKey, 64, "8ebe6a85d46737451c8bd9423f37dcb117af7316bbce1643856feeaf9f81a792ff09029e9ab1796b193eb477f938af3465f911574c57161326b71aaf0221f341", 128);
#endif

        BRKey* remoteKey = malloc(sizeof(BRKey));
        remoteKey->pubKey[0] = 0x04;
        memcpy(&remoteKey->pubKey[1], pubKey, 64);
        remoteKey->compressed = 0;
        config.remoteKey = remoteKey;
        array_add(manager->peers, config);
        ret = ETHEREUM_BOOLEAN_TRUE;
    }
    return ret;
}


//
// Public Functions
//
BREthereumNodeManager ethereumNodeManagerCreate(BREthereumNetwork network,
                                                BRKey* key,
                                                BREthereumHash headHash,
                                                uint64_t headNumber,
                                                UInt256 headTotalDifficulty,
                                                BREthereumHash genesisHash,
                                                BREthereumSubProtoCallbacks  subprotoCallbacks){

    BREthereumNodeManager manager= (BREthereumNodeManager) calloc(1, sizeof (struct BREthereumNodeManagerContext));
    
    if(manager != NULL) {
        array_new(manager->connectedNodes, ETHEREUM_PEER_MAX_CONNECTIONS);
        array_new(manager->peers, PEERS_THRESHOLD);
        pthread_mutex_init(&manager->lock, NULL);
        manager->network = network;
        manager->headHash = headHash;
        manager->headNumber = headNumber;
        manager->headTotalDifficulty = headTotalDifficulty;
        manager->genesisHash = genesisHash;
        manager->subprotoCallbacks = subprotoCallbacks;
        manager->randomContext = ethereumRandomCreate(key->secret.u8, 32);
        manager->managerCallbacks.info = manager; 
        manager->managerCallbacks.connectedFuc = _connectedCallback;
        manager->managerCallbacks.disconnectFunc = _disconnectCallback;
        manager->managerCallbacks.receivedMsgFunc = _receivedMessageCallback;
        manager->managerCallbacks.networkReachableFunc = _networkReachableCallback;
    }
    return manager;
}
void ethereumNodeManagerRelease(BREthereumNodeManager manager) {
    assert(manager != NULL);
    array_free(manager->connectedNodes);
    array_free(manager->peers);
    ethereumRandomRelease(manager->randomContext);
    free(manager);
}
BREthereumNodeManagerStatus ethereumNodeManagerStatus(BREthereumNodeManager manager){
    assert(manager != NULL);
    BREthereumNodeManagerStatus retStatus = BRE_MANAGER_DISCONNECTED;
    pthread_mutex_lock(&manager->lock);
    for(int i = 0; i < array_count(manager->connectedNodes); ++i) {
    
        BREthereumNodeStatus status = ethereumNodeStatus(manager->connectedNodes[i]);
        
        if(status == BRE_NODE_CONNECTED) {
            retStatus = BRE_MANAGER_CONNECTED;
            break;
        }
        else if (status == BRE_NODE_CONNECTING) {
            retStatus = BRE_MANAGER_CONNECTING;
        }
    }
    pthread_mutex_unlock(&manager->lock);
    return retStatus;
}
int ethereumNodeMangerConnect(BREthereumNodeManager manager) {
    assert(manager != NULL);
        
    pthread_mutex_lock(&manager->lock);
    int retValue = 0;
    int connectedCount = 0;
    if(array_count(manager->connectedNodes) < ETHEREUM_PEER_MAX_CONNECTIONS)
    {
        if(ETHEREUM_BOOLEAN_IS_TRUE(_findPeers(manager))){
            
            int peerIdx = 0;
            while(array_count(manager->peers) > 0 && array_count(manager->connectedNodes) < ETHEREUM_PEER_MAX_CONNECTIONS)
            {
                BRKey* nodeKey = (BRKey*)calloc(1,sizeof(BRKey));
                BRKey* ephemeralKey = (BRKey*)calloc(1,sizeof(BRKey));
                UInt256* nonce = (UInt256*)calloc(1,sizeof(UInt256));
            
                ethereumRandomGenPriKey(manager->randomContext, nodeKey);
                ethereumRandomGenPriKey(manager->randomContext, ephemeralKey);
                ethereumRandomGenUInt256(manager->randomContext, nonce);
            
                BREthereumNode node = ethereumNodeCreate(manager->peers[peerIdx], nodeKey, nonce, ephemeralKey, manager->managerCallbacks, ETHEREUM_BOOLEAN_TRUE);
                if(ethereumNodeConnect(node))
                {
                   //We could not connect to the remote peer so free the memory
                   ethereumNodeRelease(node);
                }
                else
                {
                  connectedCount++;
                  array_add(manager->connectedNodes, node);
                }
                array_rm(manager->peers, peerIdx);
            }
        }
        else
        {
            eth_log(ETH_LOG_TOPIC, "%s", "Could not find any remote peers to connect to");
            retValue = 1;
        }
    }
    pthread_mutex_unlock(&manager->lock);
    
    if(connectedCount <= 0) {
        eth_log(ETH_LOG_TOPIC, "%s", "Could not succesfully open a connection to any remote peers");
        retValue = 1;
    }
    return retValue;
}
void ethereumNodeManagerDisconnect(BREthereumNodeManager manager) {
    assert(manager != NULL);
    pthread_mutex_lock(&manager->lock);
    for(int i = 0; i < array_count(manager->connectedNodes); ++i) {
        ethereumNodeDisconnect(manager->connectedNodes[i], BRE_UNKNOWN);
    }
    array_clear(manager->connectedNodes);
    pthread_mutex_unlock(&manager->lock);
}
BREthereumBoolean ethereumNodeManagerSendMessage(BREthereumNodeManager manager, uint64_t packetType, uint8_t* payload, size_t payloadSize){
    
    assert(manager != NULL);
    pthread_mutex_lock(&manager->lock);
    BREthereumBoolean retStatus = ETHEREUM_BOOLEAN_FALSE;
    for(int i = 0; i < array_count(manager->connectedNodes); ++i) {
        if(ETHEREUM_BOOLEAN_IS_TRUE(ethereumNodeSendMessage(manager->connectedNodes[i], packetType, payload, payloadSize))){
            retStatus = ETHEREUM_BOOLEAN_TRUE;
        }
    }
    pthread_mutex_unlock(&manager->lock);
    return retStatus;
}
