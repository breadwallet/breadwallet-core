//
//  BREthereumLES.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 5/01/18.
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
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "BREthereumLES.h"
#include "BREthereumLESCoder.h"
#include "../base/BREthereumBase.h"
#include "../blockchain/BREthereumTransaction.h"
#include "../blockchain/BREthereumNetwork.h"
#include "BRArray.h"
#include "BREthereumNodeManager.h"
#include "../rlp/BRRlp.h"
#include "../util/BRUtil.h"
#include "BRInt.h"

#define ETH_LOG_TOPIC "BREthereumLES"

static BREthereumPeerConfigInfo _lesBRDTestnetPeerInfo[1] = {{"9683a29b13c7190cfd63cccba4bcb62d7b710da0b1c4bff2c4b8bcf129127d7b3f591163a58449d7f66200db3c208d06b9e9a8bea69be4a72e1728a83d703063",
                                                       "35.226.161.198",
                                                        30303,
                                                        30303}};

static BREthereumPeerConfigInfo _lesBRDMainnetPeerInfo[2] = {{"e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851",
                                                       "104.197.99.24",
                                                        30303,
                                                        30303},
                                                      {"e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851",
                                                       "35.184.255.33",
                                                        30303,
                                                        30303}};

static BREthereumPeerConfigInfo _lesPubMainnetPeerInfo[27] = {{"03f178d5d4511937933b50b7af683b467abaef8cfc5f7c2c9b271f61e228578ae192aaafc7f0d8035dfa994e734c2c2f72c229e383706be2f4fa43efbe9f94f4",
                                                         "163.172.149.200",
                                                         30303,
                                                         30303},
                                                        {"15a479bff68597c7d06462872842d5d18c92a0cd169b3c089aae8ffebec30086768d21d9397865b4a787ad571c5d440b412168a68d10a7e9ce3bf7cdcead6ecd",
                                                         "139.162.119.5",
                                                         30303,
                                                         30303},
                                                        {"16d92fc94f4ec4386aca44d255853c27cbe97a4274c0df98d2b642b0cc4b2f2330e99b00b46db8a031da1a631c85e2b4742d52f5eaeca46612cd28db41fb1d7f",
                                                         "91.223.175.173",
                                                         30303,
                                                         30303},
                                                        {"1a31d4480f9298f5c88bee63c62b58b7c2ec8c79b305f04fcd10cfb53459a51f9ace2cf91c9d9b6b91499a7b85c15dbe25a2fc4a5177a1f72b2ba95f1d24b73a",
                                                         "109.232.77.19",
                                                         30303,
                                                         30303},
                                                        {"1d70e87a2ee28a2762f1b2cd56f1b9134824a84264030539bba297f67a5bc9ec7ae3016b5f900dc59b1c27b4e258a63fc282a37b2dd6e25a8377473530513394",
                                                         "208.88.169.151",
                                                         30303,
                                                         30303},
                                                        {"2097dc8e40ca43a681fef2a0bf7e64126ebcc82b3573a27a3a72d533aafe24efaf5d417b159e4daed37ef7daad9cfa79aca089cb7c9caff6ef936bb289db71ca",
                                                         "109.232.77.12",
                                                         30303,
                                                         30303},
                                                        {"251d855cf9206425fa1fd0cee06210894d492134658126379bcea80a1e1c7f5efeca93510efcc4f4750459ad0bf2acaaabb5d8bace77adddf26f0b538f5de2f2",
                                                         "109.232.77.17",
                                                         30303,
                                                         30303},
                                                        {"31b5db1136a0ebceeb0ab6879e95dc66e8c52bcce9c8de50e2f722b5868f782aa0306b6b137b9e0c6271a419c5562a194d7f2abd78e22dcd1f55700dfc30c46a",
                                                         "35.165.17.127",
                                                         30303,
                                                         30303},
                                                        {"35e5bae6f4a72588ed998bbe436c16d41a75d310e9ac59132f70f3d9b43e700e1acd34cca4a3fc805e9f8de2f214ab96fdea3ccc7b9be5f3f96f638d2bd7fb6c",
                                                         "109.232.77.20",
                                                         30303,
                                                         30303},
                                                        {"3afdfd40713a8b188a94e4c7a9ddc61bc6ef176c3abbb13d1dd35eb367725b95329a7570039044dbffa49c50d4aa65f0a1f99ee68e46b8e2f09100d11d4fc85a",
                                                         "31.16.0.92",
                                                         30303,
                                                         30303},
                                                        {"3e9301c797f3863d7d0f29eec9a416f13956bd3a14eec7e0cf5eb56942841526269209edf6f57cd1315bef60c4ebbe3476bc5457bed4e479cac844c8c9e375d3",
                                                         "109.232.77.21",
                                                         30303,
                                                         30303},
                                                        {"44c67e74ba7abdc9a8c53679621d1364faac585d7af873d553c81263d3c0b3ba322130c225c680b52f8b8a140c05df796583e51c3521fcfe4c5b1320cb6097a7",
                                                         "109.232.77.11",
                                                         30303,
                                                         30303},
                                                        {"47cc9ddb429f783ff66f61c902e871020346a0cf2696153c2af297cd5a9beacad6a83065db87b2494db11f8002342752e128351bf0abe18b3b190a5a4e5d85d2",
                                                         "109.232.77.8",
                                                         30303,
                                                         30303},
                                                        {"5a4ed3587c4199e4fa050d0094164ee37935cd055574f56034d344f495a9b6752b303ee7251335785c36f2f82a5656f1118e95d548fd4f66173ca335480e3c17",
                                                         "89.40.10.150",
                                                         30303,
                                                         30303},
                                                        {"78de8a0916848093c73790ead81d1928bec737d565119932b98c6b100d944b7a95e94f847f689fc723399d2e31129d182f7ef3863f2b4c820abbf3ab2722344d",
                                                         "191.235.84.50",
                                                         30303,
                                                         30303},
                                                         {"89495deb21261a4542d50167d6e69cf3b1a585609e6843a23becbd349d92755bd2ddcc55bb1f2c017099b774454d95ef5ebccbed1859fc530fb34843ddfc32e2",
                                                         "52.39.91.131",
                                                         30303,
                                                         30303},
                                                        {"95176fe178be55d40aae49a5c11f21aa58968e13c681a6b1f571b2bb3e45927a7fb3888361bef85c0e28a52ea0e4afa17dcaa9d6c61baf504b3559f056f78581",
                                                         "163.172.145.241",
                                                         30303,
                                                         30303},
                                                        {"954481639520d993ddd6e02a5a9204d66a6bdd43809f4ca6add188851ceee7a4daf979945aaa6a58cf8905f4a45de49c8a4721de7be8107216f29245a367cdf9",
                                                         "109.232.77.24",
                                                         30303,
                                                         30303},
                                                        {"9e0b96d69ac772f00de164f25c42dc88d8096e0f71dc9606ff2aeabc6bda5a3cfe14064da241a0aa9267d9ebad82562012b32678f6807515f1802d613ee5f74a",
                                                         "109.232.77.9",
                                                         30303,
                                                         30303},
                                                        {"af8983f0592c5f6938ad9e3014985ad87e51f0ca4d10094fa6a3c05851e7a14969de3dee7f800b0a7715245c7ee5bbb6ec8a7cf217401f741d061e50f3fc6437",
                                                         "109.232.77.27",
                                                         30303,
                                                         30303},
                                                        {"bdb0d4eb53941022f6b6eab37761941087892b2fe23a1f115d386009b047a2d5ff62ab0620c187f77a5c0a0cf6dfb793275467c264b9b8368cdaa7334c4cf8e6",
                                                         "109.232.77.15",
                                                         30303,
                                                         30303},
                                                        {"d17489a8fbdb16587fce5cea0383a3b2e87c53d01eddbd4b2a8ba1b4af353c1bdd9551f8894acfa0d770aaa7eca9e334e15a304a356b4ec214a274f60f2435d6",
                                                         "107.188.141.56",
                                                         30303,
                                                         30303},
                                                        {"d679ade95bdd30c9277f492a59a34ed3bb6d98f262ac87b9a768732232167b3a7b2b815e4bd4f6ae39824dd89d75c7149a46fd895384966d355df70139874b73",
                                                         "109.232.77.29",
                                                         30303,
                                                         30303},
                                                        {"e70d9a9175a2cd27b55821c29967fdbfdfaa400328679e98ed61060bc7acba2e1ddd175332ee4a651292743ffd26c9a9de8c4fce931f8d7271b8afd7d221e851",
                                                         "35.226.238.26",
                                                         30303,
                                                         30303},
                                                        {"ea1737bf696928b4b686a2ccf61a6f2295d149281a80b0d83a9bce242e7bb084434c0837a2002d4cc2840663571ecf3e45517545499c466e4373c69951d090fe",
                                                         "163.172.181.92",
                                                         30303,
                                                         30303},
                                                        {"f251404ab66f10df6f541d69d735616a7d78e04673ec40cdfe6bf3d1fb5d84647ba627f22a1e8c5e2aa45629c88e33bc394cc1633a63fed11d84304892e51fe9",
                                                         "196.54.41.29",
                                                         24900,
                                                         30303},
                                                        {"f8ff6758d6b9e5b239051f17648ef57d63b589353b22dd5b53d34b1340b29d55b481259252af7ae490510a3a7dbc1be47222c315033d66db320df8769a1bfdd9",
                                                         "109.232.77.10",
                                                         30303,
                                                         30303}};

typedef struct {
  uint64_t requestId;
  union
  {
     struct {
         BREthereumLESTransactionStatusCallback callback;
         BREthereumLESTransactionStatusContext ctx;
         BREthereumHash* transactions;
         size_t transactionsSize;
     }transaction_status;
     
     struct {
        BREthereumLESBlockHeadersCallback callback;
        BREthereumLESBlockHeadersContext context;
        BREthereumBlockHeader header;
     }block_headers;
     
     struct {
        BREthereumLESBlockBodiesCallback callback;
        BREthereumLESBlockBodiesContext context;
        BREthereumHash* blocks;
        BREthereumTransaction* transaction;
        BREthereumHash* ommer;
     }block_bodies;
     
     struct {
        BREthereumLESReceiptsCallback callback;
        BREthereumLESBlockBodiesContext context;
        BREthereumHash* blocks;
        BREthereumTransactionReceipt* receipt;
     }receipts;
     
     struct {
        BREthereumLESProofsV2Callback callback;
        BREthereumLESProofsV2Context context;
        BREthereumProofsRequest* proofRequests;
     }proofsV2;
  }u;
}LESRequestRecord;

struct BREthereumLESContext {
    BREthereumLESAnnounceContext announceCtx;
    BREthereumLESAnnounceCallback announceFunc;
    BREthereumNodeManager nodeManager;
    BRKey* key;
    BREthereumNetwork network;
    BREthereumLESStatusMessage peerStatus;
    BREthereumLESStatusMessage statusMsg;
    BREthereumBoolean startSendingMessages;
    BREthereumSubProtoCallbacks callbacks;
    BREthereumLESStatus status;
    LESRequestRecord* requests;
    uint64_t message_id_offset;
    uint64_t requestIdCount;
    pthread_mutex_t lock;
    pthread_cond_t startCond;
};
static int _findRequestId(BREthereumLES les, uint64_t reqId, uint64_t* reqLocIndex){

    int ret = 0;

    pthread_mutex_lock(&les->lock);
    for(int i = 0; i < array_count(les->requests); i++) {
        if(reqId == les->requests[i].requestId){
            *reqLocIndex = i;
            ret = 1;
            break;
        }
    }
    pthread_mutex_unlock(&les->lock);
    
    return ret;
}
static void _receivedMessageCallback(BREthereumSubProtoContext info, uint64_t messageType, BRRlpData messageBody) {

    BREthereumLES les = (BREthereumLES)info;

    switch (messageType - les->message_id_offset)
    {
        case BRE_LES_ID_STATUS:
        {
            BREthereumLESDecodeStatus remoteStatus = ethereumLESDecodeStatus(messageBody.bytes, messageBody.bytesCount, &les->peerStatus);
            if(remoteStatus == BRE_LES_CODER_SUCCESS)
            {
                if(ETHEREUM_BOOLEAN_IS_TRUE(les->peerStatus.txRelay) &&
                  les->peerStatus.chainId == networkGetChainId(les->network)){
                    eth_log(ETH_LOG_TOPIC, "%s", "LES Handshake complete. Start sending messages");
                    les->startSendingMessages = ETHEREUM_BOOLEAN_TRUE;
                }else {
                    eth_log(ETH_LOG_TOPIC, "%s", "Disconnecting from node. Does not meet the requirements for LES");
                }
            }
        }
        break;
        case BRE_LES_ID_ANNOUNCE:
        {
            BREthereumHash hash;
            uint64_t headNumber;
            UInt256 headTd;
            uint64_t reorgDepth;
            BREthereumLESDecodeStatus remoteStatus = ethereumLESDecodeAnnounce(messageBody.bytes, messageBody.bytesCount, &hash, &headNumber, &headTd, &reorgDepth, &les->peerStatus);
            if(remoteStatus == BRE_LES_CODER_SUCCESS)
            {
                eth_log(ETH_LOG_TOPIC, "%s", "Received Announce message from Remote peer");
                les->announceFunc(les->announceCtx, hash, headNumber, headTd);
                //TODO: Check to make sure peerStatus is valid after update. 
            }
        }
        break;
        case BRE_LES_ID_TX_STATUS:
        {
            //rlpShow(messageBody, "LES-TX_STATUS");
            uint64_t reqId = 0, bv = 0;
            BREthereumTransactionStatus* replies;
            size_t repliesCount;
            BREthereumLESDecodeStatus status = ethereumLESDecodeTxStatus(messageBody.bytes, messageBody.bytesCount, &reqId, &bv, &replies, &repliesCount);
            
            if(status == BRE_LES_CODER_SUCCESS)
            {
                eth_log(ETH_LOG_TOPIC, "%s", "Received [Tranactions Status] reply from remote peer");
                
                uint64_t requestIndexRm = 0;
                
                if(_findRequestId(les, reqId, &requestIndexRm)){
                    pthread_mutex_lock(&les->lock);
                    for(int i = 0; i < repliesCount; ++i){
                        les->requests[requestIndexRm].u.transaction_status.callback(les->requests[requestIndexRm].u.transaction_status.ctx,
                                                                                    les->requests[requestIndexRm].u.transaction_status.transactions[i],
                                                                                    replies[i]);
                    }
                    free(les->requests[requestIndexRm].u.transaction_status.transactions);
                    array_rm(les->requests, requestIndexRm);
                    free (replies);
                    pthread_mutex_unlock(&les->lock);
                }
            }
        }
        break;
        case BRE_LES_ID_BLOCK_HEADERS:
        {
//            rlpShow(messageBody, "LES-HEADERS");
            uint64_t reqId = 0, bv = 0;
            BREthereumBlockHeader* headers;
            BREthereumLESDecodeStatus status = ethereumLESDecodeBlockHeaders(messageBody.bytes, messageBody.bytesCount, &reqId, &bv, &headers); 
            
            if(status == BRE_LES_CODER_SUCCESS)
            {
                eth_log(ETH_LOG_TOPIC, "%s BV:%llu", "Received [Block Headers] reply from remote peer", bv);
                
                uint64_t requestIndexRm = 0;
                
                if(_findRequestId(les, reqId, &requestIndexRm)){

                    BREthereumLESBlockHeadersCallback callback = les->requests[requestIndexRm].u.block_headers.callback;
                    BREthereumLESBlockHeadersContext context = les->requests[requestIndexRm].u.block_headers.context;
                    
                    for(int i = 0; i < array_count(headers); ++i){
                        callback(context, headers[i]);
                    }

                    pthread_mutex_lock(&les->lock);
                    array_rm(les->requests, requestIndexRm);
                    pthread_mutex_unlock(&les->lock);
                }
                array_free (headers);
            }
    
        }
        break;
        case BRE_LES_ID_BLOCK_BODIES:
        {
           // rlpShow(messageBody, "LES-BODIES");
            uint64_t reqId = 0, bv = 0;
            
            BREthereumBlockHeader** ommers;
            BREthereumTransaction** transactions;
            BREthereumLESDecodeStatus status = ethereumLESDecodeBlockBodies(messageBody.bytes, messageBody.bytesCount, &reqId, &bv, les->network, &ommers, &transactions);
            
            
            if(status == BRE_LES_CODER_SUCCESS)
            {
                eth_log(ETH_LOG_TOPIC, "%s BV:%llu", "Received [Block Blodies] reply from remote peer", bv);
                
                uint64_t requestIndexRm = 0;
                if(_findRequestId(les, reqId, &requestIndexRm)){
                

                    BREthereumLESBlockBodiesCallback callback = les->requests[requestIndexRm].u.block_bodies.callback;
                    BREthereumLESBlockBodiesContext context = les->requests[requestIndexRm].u.block_bodies.context;
                    BREthereumHash* blocks = les->requests[requestIndexRm].u.block_bodies.blocks;
                    
                    for(int i = 0; i < array_count(blocks); ++i){
                        callback(context, blocks[i],transactions[i],ommers[i]);
                    }
                    
                    pthread_mutex_lock(&les->lock);
                    array_free(les->requests[requestIndexRm].u.block_bodies.blocks);
                    array_rm(les->requests, requestIndexRm);
                    pthread_mutex_unlock(&les->lock);
                }
                array_free (ommers);
                array_free (transactions);
            } 
        
        }
        break;
        case BRE_LES_ID_RECEIPTS:
        {
           // size_t len = 0;
           // printf("%s", encodeHexCreate(&len, messageBody.bytes, messageBody.bytesCount));
            // rlpShow(messageBody, "LES-RECEIPTS");
            uint64_t reqId = 0, bv = 0;
            BREthereumTransactionReceipt** receipts;
            BREthereumLESDecodeStatus status = ethereumLESDecodeReceipts(messageBody.bytes, messageBody.bytesCount, &reqId, &bv, &receipts);
        
            if(status == BRE_LES_CODER_SUCCESS)
            {
                eth_log(ETH_LOG_TOPIC, "%s", "Received [RECEIPTS] reply from remote peer");
                
                uint64_t requestIndexRm = 0;
                _findRequestId(les, reqId, &requestIndexRm);
                
                if(_findRequestId(les, reqId, &requestIndexRm)) {
                    pthread_mutex_lock(&les->lock);

                    BREthereumLESReceiptsCallback callback = les->requests[requestIndexRm].u.receipts.callback;
                    BREthereumLESReceiptsContext context = les->requests[requestIndexRm].u.receipts.context;
                    BREthereumHash* blocks = les->requests[requestIndexRm].u.receipts.blocks;
                    
                    for(int i = 0; i < array_count(blocks); ++i){
                        callback(context, blocks[i],receipts[i]);
                    }
                    
                    array_free(les->requests[requestIndexRm].u.receipts.blocks);
                    array_rm(les->requests, requestIndexRm);
                    pthread_mutex_unlock(&les->lock);
                }
                array_free(receipts);
            }
           
        }
        break;
        case BRE_LES_ID_PROOFS_V2:
        {
            rlpShow(messageBody, "LES-PROOFSV2");
            uint64_t reqId = 0, bv = 0;
            BREthereumLESDecodeStatus status = BRE_LES_CODER_UNABLE_TO_DECODE_ERROR; // Call Decode once we know what is returned from the full node
        
            if(status == BRE_LES_CODER_SUCCESS)
            {
                eth_log(ETH_LOG_TOPIC, "%s", "Received [RECEIPTS] reply from remote peer");
                
                uint64_t requestIndexRm = 0;
                _findRequestId(les, reqId, &requestIndexRm);
                
                if(_findRequestId(les, reqId, &requestIndexRm)) {
                    pthread_mutex_lock(&les->lock);

                    BREthereumLESProofsV2Callback callback = les->requests[requestIndexRm].u.proofsV2.callback;
                    BREthereumLESProofsV2Context context = les->requests[requestIndexRm].u.proofsV2.context;
                    BREthereumProofsRequest* proofRequests = les->requests[requestIndexRm].u.proofsV2.proofRequests;
                    
                    for(int i = 0; i < array_count(proofRequests); ++i){
                        //Call the callback function once we know what we are getting back. 
                    }
                    
                    array_free(les->requests[requestIndexRm].u.proofsV2.proofRequests);
                    array_rm(les->requests, requestIndexRm);
                    pthread_mutex_unlock(&les->lock);
                }
            }
           
        }
        break;
        default:
        {
            eth_log(ETH_LOG_TOPIC, "Received Message that cannot be handled, message id:%llu", messageType - les->message_id_offset);
        }
        break;
    }
}
static void _connectedToNetworkCallback(BREthereumSubProtoContext info, uint8_t** statusBytes, size_t* statusSize){
    BREthereumLES les = (BREthereumLES)info;
    BRRlpData statusPayload = ethereumLESEncodeStatus(les->message_id_offset, &les->statusMsg);
    *statusBytes = statusPayload.bytes;
    *statusSize = statusPayload.bytesCount;
}
static void _networkReachableCallback(BREthereumSubProtoContext info, BREthereumBoolean isReachable) {}

static BREthereumLESStatus _sendMessage(BREthereumLES les, uint8_t packetType, BRRlpData payload) {

    BREthereumNodeManagerStatus status = ethereumNodeManagerStatus(les->nodeManager);
    BREthereumLESStatus retStatus = LES_NETWORK_UNREACHABLE;
    
    if(ETHEREUM_BOOLEAN_IS_TRUE(les->startSendingMessages)){
      
       if(status == BRE_MANAGER_CONNECTED)
        {
            if(ETHEREUM_BOOLEAN_IS_TRUE(ethereumNodeManagerSendMessage(les->nodeManager, packetType, payload.bytes, payload.bytesCount))){
                retStatus = LES_SUCCESS;
            }
        }
        else
        {
            //Retry to connect to the ethereum network
            if(ethereumNodeMangerConnect(les->nodeManager)){
                sleep(3); //Give it some time to see if the node manager can connect to the network again;
                if(ETHEREUM_BOOLEAN_IS_TRUE(ethereumNodeManagerSendMessage(les->nodeManager, packetType, payload.bytes, payload.bytesCount))){
                    retStatus = LES_SUCCESS;
                }
            }
        }
    }
    return retStatus;
}
//
// Public functions
//
extern BREthereumLES
lesCreate (BREthereumNetwork network,
           BREthereumLESAnnounceContext announceContext,
           BREthereumLESAnnounceCallback announceCallback,
           BREthereumHash headHash,
           uint64_t headNumber,
           UInt256 headTotalDifficulty,
           BREthereumHash genesisHash) {
    
    BREthereumLES les = (BREthereumLES) calloc(1,sizeof(struct BREthereumLESContext));
    
    if(les != NULL)
    {
        les->requestIdCount = 0;
        les->callbacks.info = les;
        les->callbacks.connectedFunc = _connectedToNetworkCallback;
        les->callbacks.messageRecFunc = _receivedMessageCallback;
        les->callbacks.networkReachFunc = _networkReachableCallback;
        les->key = (BRKey *)malloc(sizeof(BRKey));
        uint8_t data[32] = {3,4,5,0};
        memcpy(les->key->secret.u8,data,32);
        les->key->compressed = 0;
        les->startSendingMessages = ETHEREUM_BOOLEAN_TRUE;
        les->network = network;
        
        /*** Define the status message **/
        les->statusMsg.protocolVersion = 0x02;
        les->statusMsg.chainId = networkGetChainId(network);
        les->statusMsg.headerTd = headTotalDifficulty;
        memcpy(les->statusMsg.headHash, headHash.bytes, 32);
        les->statusMsg.headNum = headNumber;
        memcpy(les->statusMsg.genesisHash, genesisHash.bytes, 32);
        les->statusMsg.serveHeaders = ETHEREUM_BOOLEAN_FALSE;
        les->statusMsg.serveChainSince = NULL;
        les->statusMsg.serveStateSince = NULL;
        les->statusMsg.txRelay = ETHEREUM_BOOLEAN_FALSE;
        les->statusMsg.flowControlBL = NULL;
        les->statusMsg.flowControlMRC = NULL;
        les->statusMsg.flowControlMRR = NULL;
        les->statusMsg.flowControlMRCCount = NULL;
        les->statusMsg.announceType = 1;

        //Assign announce information
        les->announceCtx = announceContext;
        les->announceFunc = announceCallback;
        
        //Assign the message id offset for now to be 0x10 since we only support LES
        les->message_id_offset = 0x10;

        //Define the Requests Array
        array_new(les->requests,100);
        les->nodeManager = NULL;
        pthread_mutex_init(&les->lock, NULL);
        pthread_cond_init(&les->startCond, NULL);
        ethereumNodeMangerConnect(les->nodeManager);
        
    }
    return les;
}
extern BREthereumBoolean lesStart(BREthereumLES les, unsigned int maxWaitTime) {

    assert(les != NULL);
    
    pthread_muxtex_lock(&les->lock);
    if(les->nodeManager == NULL) {
        les->nodeManager = ethereumNodeManagerCreate(network, les->key, headHash, headNumber, les->statusMsg.headerTd, genesisHash, les->callbacks);
        ethereumNodeMangerConnect(les->nodeManager);
    
    }
    pthread_mutex_unlock(&les->lock);

}
extern void lesStop(BREthereumLES les) {




}
extern void lesRelease(BREthereumLES les) {
    ethereumNodeManagerDisconnect(les->nodeManager); 
    ethereumNodeManagerRelease(les->nodeManager);
    free(les);
}


//
// LES messages
//
static void _addTxtStatusRecord(
                         BREthereumLES les,
                         BREthereumLESTransactionStatusContext context,
                         BREthereumLESTransactionStatusCallback callback,
                         BREthereumHash transactions[],
                         uint64_t reqId) {

    LESRequestRecord record;
    record.requestId = reqId;
    record.u.transaction_status.callback = callback;
    record.u.transaction_status.ctx = context;
    record.u.transaction_status.transactions = malloc(sizeof(BREthereumHash) * array_count(transactions));
    record.u.transaction_status.transactionsSize = array_count(transactions);
    for(int i = 0; i < array_count(transactions); ++i) {
        memcpy(record.u.transaction_status.transactions[i].bytes, transactions[i].bytes, sizeof(transactions[0].bytes));
    }
    
    pthread_mutex_unlock(&les->lock);
    array_add(les->requests, record);
    pthread_mutex_unlock(&les->lock);
}

extern BREthereumLESStatus
lesGetTransactionStatus (BREthereumLES les,
                         BREthereumLESTransactionStatusContext context,
                         BREthereumLESTransactionStatusCallback callback,
                         BREthereumHash transactions[]) {
                         
    BREthereumBoolean shouldSend;
    pthread_mutex_lock(&les->lock);
    shouldSend = les->startSendingMessages;
    uint64_t reqId = les->requestIdCount++;
    pthread_mutex_unlock(&les->lock);
    if(ETHEREUM_BOOLEAN_IS_TRUE(shouldSend))
    {
        
        _addTxtStatusRecord(les, context, callback, transactions, reqId);

        BRRlpData txtStatusData = ethereumLESGetTxStatus(les->message_id_offset, reqId, transactions);
        BREthereumLESStatus status =  _sendMessage(les, BRE_LES_ID_GET_TX_STATUS, txtStatusData);
        rlpDataRelease(txtStatusData);
        return status;
    }
    else {
        return LES_NETWORK_UNREACHABLE;
    }
}

extern BREthereumLESStatus
lesGetTransactionStatusOne (BREthereumLES les,
                            BREthereumLESTransactionStatusContext context,
                            BREthereumLESTransactionStatusCallback callback,
                            BREthereumHash transaction) {
    
    BREthereumHash* transactionArr;
    array_new(transactionArr, 1);
    array_add(transactionArr, transaction);
    
    BREthereumLESStatus status = lesGetTransactionStatus(les,context,callback,transactionArr);
    
    array_free(transactionArr);
    
    return status;
}

extern BREthereumLESStatus
lesSubmitTransaction (BREthereumLES les,
                      BREthereumLESTransactionStatusContext context,
                      BREthereumLESTransactionStatusCallback callback,
                      BREthereumTransaction transaction) {
    
    BREthereumBoolean shouldSend;

    pthread_mutex_lock(&les->lock);
    uint64_t reqId = les->requestIdCount++;
    shouldSend = les->startSendingMessages; 
    pthread_mutex_unlock(&les->lock);
    
    if(ETHEREUM_BOOLEAN_IS_TRUE(shouldSend)) {
        BREthereumTransaction* transactionArr;
        array_new(transactionArr, 1);
        array_add(transactionArr, transaction);
        
        BRRlpData sendTxtData;
        if(les->peerStatus.protocolVersion == 0x02) {
            sendTxtData = ethereumLESSendTxtV2(les->message_id_offset, reqId, transactionArr, les->network, RLP_TYPE_TRANSACTION_SIGNED);
            BREthereumHash* transactionHashArr;
            array_new(transactionHashArr, 1);
            array_add(transactionHashArr, transactionGetHash(transaction));
            _addTxtStatusRecord(les, context, callback, transactionHashArr, reqId);
           array_free(transactionHashArr);
        }else {
            sendTxtData = ethereumLESSendTxt(les->message_id_offset, reqId, transactionArr, les->network, RLP_TYPE_TRANSACTION_SIGNED);
        }
        BREthereumLESStatus status =  _sendMessage(les, BRE_LES_ID_SEND_TX2, sendTxtData);
        array_free(transactionArr);
        rlpDataRelease(sendTxtData);
        return status;
    }
    else {
        return LES_NETWORK_UNREACHABLE;
    }
}
extern BREthereumLESStatus
lesGetReceipts (BREthereumLES les,
                BREthereumLESReceiptsContext context,
                BREthereumLESReceiptsCallback callback,
                BREthereumHash blocks[]) {
    
    BREthereumBoolean shouldSend;

    pthread_mutex_lock(&les->lock);
    uint64_t reqId = les->requestIdCount++;
    shouldSend = les->startSendingMessages;
    pthread_mutex_unlock(&les->lock);
    
    if(ETHEREUM_BOOLEAN_IS_TRUE(shouldSend)) {
        BRRlpData blockBodiesData = ethereumLESGetReceipts(les->message_id_offset, reqId, blocks);
        
        LESRequestRecord record;
        record.requestId = reqId;
        record.u.receipts.callback = callback;
        record.u.receipts.context = context;
        array_new(record.u.receipts.blocks, array_count(blocks));
        array_add_array(record.u.receipts.blocks, blocks, array_count(blocks));
        
        pthread_mutex_unlock(&les->lock);
        array_add(les->requests, record);
        pthread_mutex_unlock(&les->lock);
        
        BREthereumLESStatus status =  _sendMessage(les, BRE_LES_ID_GET_RECEIPTS, blockBodiesData);
        rlpDataRelease(blockBodiesData);
        return status;
    }
    else {
        return LES_NETWORK_UNREACHABLE;
    }
    
    
    
}

extern BREthereumLESStatus
lesGetReceiptsOne (BREthereumLES les,
                   BREthereumLESReceiptsContext context,
                   BREthereumLESReceiptsCallback callback,
                   BREthereumHash block) {
    
    BREthereumHash* array;
    array_new(array, 1);
    array_add(array, block);
    
    BREthereumLESStatus status = lesGetReceipts(les,context,callback,array);
    
    array_free(array);
    
    return status;
}
extern BREthereumLESStatus
lesGetGetProofsV2One (BREthereumLES les,
                     BREthereumLESProofsV2Context context,
                     BREthereumLESProofsV2Callback callback,
                     BREthereumHash blockHash,
                     BREthereumHash  key,
                     BREthereumHash key2,
                     uint64_t fromLevel) {
    
    BREthereumBoolean shouldSend;

    pthread_mutex_lock(&les->lock);
    uint64_t reqId = les->requestIdCount++;
    shouldSend = les->startSendingMessages;
    pthread_mutex_unlock(&les->lock);
    
    if(ETHEREUM_BOOLEAN_IS_TRUE(shouldSend)) {
        BREthereumProofsRequest* proofs;
        BREthereumProofsRequest request;
        memcpy(request.blockHash.bytes, blockHash.bytes, 32);
        memcpy(request.key2.bytes, key2.bytes, 32);
        memcpy(request.key.bytes, key.bytes, 32);
        request.fromLevel = fromLevel;
        array_new(proofs, 1);
        array_add(proofs, request);
        
        LESRequestRecord record;
        record.requestId = reqId;
        record.u.proofsV2.callback = callback;
        record.u.proofsV2.context = context;
        record.u.proofsV2.proofRequests = proofs;
        
        pthread_mutex_unlock(&les->lock);
        array_add(les->requests, record);
        pthread_mutex_unlock(&les->lock);
        
        
        BRRlpData proofsData = ethereumLESGetProofsV2(les->message_id_offset, reqId, proofs);
        BREthereumLESStatus status =  _sendMessage(les, BRE_LES_ID_GET_PROOFS_V2, proofsData);
        rlpDataRelease(proofsData);
        return status;
    }
    else {
        return LES_NETWORK_UNREACHABLE;
    }
    
}

extern BREthereumLESStatus
lesGetBlockBodies (BREthereumLES les,
                   BREthereumLESBlockBodiesContext context,
                   BREthereumLESBlockBodiesCallback callback,
                   BREthereumHash blocks[]) {
    
    BREthereumBoolean shouldSend;

    pthread_mutex_lock(&les->lock);
    uint64_t reqId = les->requestIdCount++;
    shouldSend = les->startSendingMessages;
    pthread_mutex_unlock(&les->lock);
    
    if(ETHEREUM_BOOLEAN_IS_TRUE(shouldSend)) {
    
        LESRequestRecord record;
        record.requestId = reqId;
        record.u.block_bodies.callback = callback;
        record.u.block_bodies.context = context;
        array_new(record.u.block_bodies.blocks, array_count(blocks));
        array_add_array(record.u.block_bodies.blocks, blocks, array_count(blocks));
        
        pthread_mutex_unlock(&les->lock);
        array_add(les->requests, record);
        pthread_mutex_unlock(&les->lock);
        
        BRRlpData blockBodiesData = ethereumLESGetBlockBodies(les->message_id_offset, reqId, blocks);
        BREthereumLESStatus status =  _sendMessage(les, BRE_LES_ID_GET_BLOCK_BODIES, blockBodiesData);
        rlpDataRelease(blockBodiesData);
        return status;
    }
    else {
        return LES_NETWORK_UNREACHABLE;
    }
}

extern BREthereumLESStatus
lesGetBlockBodiesOne (BREthereumLES les,
                      BREthereumLESBlockBodiesContext context,
                      BREthereumLESBlockBodiesCallback callback,
                      BREthereumHash block) {
    
    BREthereumHash* array;
    array_new(array, 1);
    array_add(array, block);
    
    BREthereumLESStatus status = lesGetBlockBodies(les,context,callback,array);
    
    array_free(array);
    
    return status;
}

extern BREthereumLESStatus
lesGetBlockHeaders (BREthereumLES les,
                    BREthereumLESBlockHeadersContext context,
                    BREthereumLESBlockHeadersCallback callback,
                    uint64_t blockNumber,
                    size_t maxBlockCount,
                    uint64_t skip,
                    BREthereumBoolean reverse) {
    BREthereumBoolean shouldSend;

    pthread_mutex_lock(&les->lock);
    uint64_t reqId = les->requestIdCount++;
    pthread_mutex_unlock(&les->lock);
    shouldSend = les->startSendingMessages;
    pthread_mutex_unlock(&les->lock);

    if(ETHEREUM_BOOLEAN_IS_TRUE(shouldSend)) {
        uint64_t reverseInt = 0;
        
        if(ETHEREUM_BOOLEAN_IS_TRUE(reverse)){
            reverseInt = 1;
        }
        
        LESRequestRecord record;
        record.requestId = reqId;
        record.u.block_headers.callback = callback;
        record.u.block_headers.context = context;
        
        pthread_mutex_unlock(&les->lock);
        array_add(les->requests, record);
        pthread_mutex_unlock(&les->lock);
        
        BRRlpData blockHeadersData = ethereumLESGetBlockHeaders(les->message_id_offset, reqId, blockNumber, maxBlockCount, skip, reverseInt);
        BREthereumLESStatus status =  _sendMessage(les, BRE_LES_ID_GET_BLOCK_HEADERS, blockHeadersData);
        rlpDataRelease(blockHeadersData);
        return status;
    }else {
        return LES_NETWORK_UNREACHABLE;
    }

    return LES_UNKNOWN_ERROR;

}
