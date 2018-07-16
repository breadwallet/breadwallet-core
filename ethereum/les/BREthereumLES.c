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
                eth_log(ETH_LOG_TOPIC, "%s", "Received [Block Headers] reply from remote peer");
                
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
                eth_log(ETH_LOG_TOPIC, "%s", "Received [Block Blodies] reply from remote peer");
                
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

        les->nodeManager = ethereumNodeManagerCreate(network, les->key, headHash, headNumber, les->statusMsg.headerTd, genesisHash, les->callbacks);
        pthread_mutex_init(&les->lock, NULL);
        ethereumNodeMangerConnect(les->nodeManager);
        
    }
    return les;
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
