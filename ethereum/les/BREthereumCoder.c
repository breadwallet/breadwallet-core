//
//  BREthereumLESCoder.c
//  breadwallet
//
//  Created by Lamont Samuels on 5/29/18.
//  Copyright © 2018 breadwallet LLC. All rights reserved.
//

#include "BREthereumCoder.h"
#include "BREthereumLESBase.h"

#define TXSTATUS_INCLUDED 3
#define TXSTATUS_ERROR 4

//
// LES Reply Structures
//
/*
typedef struct {
    BREthereumBlockHeader blockHeader;
    uint8_t* node;
}BREthereumHeaderProof;

typedef struct {
    uint8_t* node;
}BREthereumProofNode;

typedef struct {
    int foo;
}BREthereumReceipt;

typedef struct {
    uint8_t** node;
    uint8_t** auxData;
}BREthereumHelperTrieProofs;
*/


//
// LES Request Structures
//
/*
typedef struct {
    UInt256 blockHash;
    UInt256 key;
    UInt256 key2;
    uint64_t fromLevel;
}BREthereumProofsRequest;

typedef struct {
    UInt256 blockHash;
    UInt256 key;
}BREthereumContractCodesRequest;

typedef struct {
    uint64_t chtNumber;
    uint64_t blockNumber;
    uint64_t fromLevel;
}BREthereumHeaderProofRequest;

typedef struct {
    uint64_t subType;
    uint64_t sectionIdx;
    uint8_t key;
    uint64_t fromLevel;
    uint64_t auxReq;
}BREthereumGetHelperTrieProofsRequest;
*/

//
// Handshake messages
// 
static void _encodeKeyValueStatus(BRRlpCoder coder, BRRlpItem* keyPair, char* key, void* value, size_t auxValueCount) {

    if(strcmp(key, "protocolVersion") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "protocolVersion");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),1);
    }else if (strcmp(key, "networkID") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "networkId");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),1);
    }else if (strcmp(key, "headTd") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "headTd");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),1);
    }else if (strcmp(key, "headHash") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "headHash");
        keyPair[1] = rlpEncodeItemBytes(coder, ((uint8_t *)value), 32);
    }else if (strcmp(key, "headNum") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "headNum");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),1);
    }else if (strcmp(key, "genesisHash") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "genesisHash");
        keyPair[1] = rlpEncodeItemBytes(coder, ((uint8_t *)value), 32);
    }else if (strcmp(key, "serveHeaders") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "serveHeaders");
    }else if (strcmp(key, "serveChainSince") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "serveChainSince");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),1);
    }else if (strcmp(key, "serveStateSince") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "serveStateSince");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),1);
    }else if (strcmp(key, "txRelay") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "txRelay");
    }else if (strcmp(key, "flowControl/BL") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/BL");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),1);
    }else if (strcmp(key, "flowControl/MRR") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/MRC");
        size_t count = auxValueCount;
        BRRlpItem mrcItems[count];
        for(int idx = 0; idx < count; ++idx){
            BRRlpItem mrcElements [3];
            BREthereumLESMRC* flowControlMRC = ((BREthereumLESMRC *)value);
            mrcElements[0] = rlpEncodeItemUInt64(coder,flowControlMRC[idx].msgCode,1);
            mrcElements[1] = rlpEncodeItemUInt64(coder,flowControlMRC[idx].baseCost,1);
            mrcElements[2] = rlpEncodeItemUInt64(coder,flowControlMRC[idx].reqCost,1);
            mrcItems[idx] = rlpEncodeListItems(coder, mrcElements, 3);
        }
        keyPair[1] = rlpEncodeListItems(coder, mrcItems, count);
    }else if (strcmp(key, "flowControl/MRR") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/MRR");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),1);
    }
}

static void _encodeStatus(BREthereumLESStatusMessage* status, BRRlpCoder coder, BRRlpItem* statusItems, int* ioIdx) {

    int curIdx = *ioIdx;

    //protocolVersion
    BRRlpItem keyPair [2];
    keyPair[0] = rlpEncodeItemString(coder, "protocolVersion");
    keyPair[1] = rlpEncodeItemUInt64(coder, status->protocolVersion,1);
    statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    
    //networkId
    keyPair[0] = rlpEncodeItemString(coder, "networkId");
    keyPair[1] = rlpEncodeItemUInt64(coder, status->chainId,1);
    statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);

    //headTd
    keyPair[0] = rlpEncodeItemString(coder, "headTd");
    keyPair[1] = rlpEncodeItemUInt256(coder, status->headerTd,1);
    statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
 
    //headHash
    keyPair[0] = rlpEncodeItemString(coder, "headHash");
    keyPair[1] = rlpEncodeItemBytes(coder, status->headHash, sizeof(status->headHash));
    statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
 
    //headNum
    keyPair[0] = rlpEncodeItemString(coder, "headNum");
    keyPair[1] = rlpEncodeItemUInt64(coder, status->headNum,1);
    statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
 
    //genesisHash
    keyPair[0] = rlpEncodeItemString(coder, "genesisHash");
    keyPair[1] = rlpEncodeItemBytes(coder, status->genesisHash, sizeof(status->genesisHash));
    statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
 
    //serveHeaders
    if(ETHEREUM_BOOLEAN_IS_TRUE(status->serveHeaders)) {
        BRRlpItem serveHeadersItem[1];
        serveHeadersItem[0] = rlpEncodeItemString(coder, "serveHeaders");
        statusItems[curIdx++] = rlpEncodeListItems(coder, serveHeadersItem, 1);
    }
    
    //serveChainSince
    if(status->serveChainSince != NULL) {
        keyPair[0] = rlpEncodeItemString(coder, "serveChainSince");
        keyPair[1] = rlpEncodeItemUInt64(coder, *(status->serveChainSince),1);
        statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    
    //serveStateSince
    if(status->serveStateSince != NULL) {
        keyPair[0] = rlpEncodeItemString(coder, "serveStateSince");
        keyPair[1] = rlpEncodeItemUInt64(coder, *(status->serveStateSince),1);
        statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    
    //txRelay
    if(ETHEREUM_BOOLEAN_IS_TRUE(status->txRelay)) {
        BRRlpItem txRelayItem[1];
        txRelayItem[0] = rlpEncodeItemString(coder, "txRelay");
        statusItems[curIdx++] = rlpEncodeListItems(coder, txRelayItem, 1);
    }
    
    //flowControl/BL
    if(status->flowControlBL != NULL) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/BL");
        keyPair[1] = rlpEncodeItemUInt64(coder, *(status->flowControlBL),1);
        statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    //flowControl/MRC
    if(status->flowControlBL != NULL) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/MRC");
        size_t count = *(status->flowControlMRCCount);
        BRRlpItem mrcItems[count];
        for(int idx = 0; idx < count; ++idx){
            BRRlpItem mrcElements [3];
            mrcElements[0] = rlpEncodeItemUInt64(coder,status->flowControlMRC[idx].msgCode,1);
            mrcElements[1] = rlpEncodeItemUInt64(coder,status->flowControlMRC[idx].baseCost,1);
            mrcElements[2] = rlpEncodeItemUInt64(coder,status->flowControlMRC[idx].reqCost,1);
            mrcItems[idx] = rlpEncodeListItems(coder, mrcElements, 3);
        }
        keyPair[1] = rlpEncodeListItems(coder, mrcItems, count);
        statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    //flowControl/MRR
    if(status->flowControlMRR != NULL) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/MRR");
        keyPair[1] = rlpEncodeItemUInt64(coder, *(status->flowControlMRR),1);
        statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    *ioIdx = curIdx;
}
static BRRlpData _encodePayloadId(BRRlpCoder coder, BRRlpData messageListData, uint64_t message_id) {

    BRRlpData idData;
    BRRlpData retData;
    
    rlpDataExtract(coder, rlpEncodeItemUInt64(coder, message_id,1),&idData.bytes, &idData.bytesCount);
    
    uint8_t * rlpData = malloc(idData.bytesCount + messageListData.bytesCount);
    
    memcpy(rlpData, idData.bytes, idData.bytesCount);
    memcpy(&rlpData[idData.bytesCount], messageListData.bytes, messageListData.bytesCount);

    retData.bytes = rlpData;
    retData.bytesCount = idData.bytesCount + messageListData.bytesCount;
    
    rlpDataRelease(idData);
    return retData;
}

//
// Status Message 
//
BRRlpData coderEncodeStatus(uint64_t message_id_offset, BREthereumLESStatusMessage* status) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem statusItems[15];
    int ioIdx = 0;
    
   // statusItems[ioIdx++] = rlpEncodeItemUInt64(coder, BRE_LES_ID_STATUS,1); Apparently the first thing in the list should not be the message id!!
    _encodeStatus(status,coder,statusItems,&ioIdx);
    
    //announceType
    if(status->protocolVersion == 0x02){
        BRRlpItem keyPair [2];
        keyPair[0] = rlpEncodeItemString(coder, "announceType");
        keyPair[1] = rlpEncodeItemUInt64(coder, status->announceType,1);
        statusItems[ioIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    BRRlpItem encoding = rlpEncodeListItems(coder, statusItems, ioIdx);
    
    BRRlpData messageListData;
    rlpDataExtract(coder, encoding, &messageListData.bytes, &messageListData.bytesCount);
    
    BRRlpData retData = _encodePayloadId(coder, messageListData, message_id_offset + BRE_LES_ID_STATUS);
    
    rlpCoderRelease(coder);
    rlpDataRelease(messageListData);

    return retData;
}
static BREthereumLESDecodeStatus _decodeStatus(BRRlpCoder coder, const BRRlpItem *items, size_t itemsCount, BREthereumLESStatusMessage* header){

    for(int i= 0; i < itemsCount; ++i) {
        size_t keyPairCount;
        const BRRlpItem *keyPairs = rlpDecodeList(coder, items[i], &keyPairCount);
        if(keyPairCount > 0){
            char * key = rlpDecodeItemString(coder, keyPairs[0]);
            if(strcmp(key, "protocolVersion") == 0) {
                header->protocolVersion = rlpDecodeItemUInt64(coder, keyPairs[1], 1);
            }else if (strcmp(key, "networkId") == 0) {
                header->chainId = rlpDecodeItemUInt64(coder, keyPairs[1], 1);
            }else if (strcmp(key, "headTd") == 0) {
                header->headerTd = rlpDecodeItemUInt256(coder, keyPairs[1], 1);
            }else if (strcmp(key, "headHash") == 0) {
                BRRlpData hashData = rlpDecodeItemBytes(coder, keyPairs[1]);
                memcpy(header->headHash, hashData.bytes, hashData.bytesCount);
                rlpDataRelease(hashData);
            }else if (strcmp(key, "announceType") == 0) {
                header->announceType = rlpDecodeItemUInt64(coder, keyPairs[1], 1);
            }else if (strcmp(key, "headNum") == 0) {
                header->headNum = rlpDecodeItemUInt64(coder, keyPairs[1], 1);
            }else if (strcmp(key, "genesisHash") == 0) {
                BRRlpData hashData = rlpDecodeItemBytes(coder, keyPairs[1]);
                memcpy(header->genesisHash, hashData.bytes, hashData.bytesCount);
                rlpDataRelease(hashData);
            }else if (strcmp(key, "serveHeaders") == 0) {
                header->serveHeaders = ETHEREUM_BOOLEAN_TRUE;
            }else if (strcmp(key, "serveChainSince") == 0) {
                header->serveChainSince = malloc(sizeof(uint64_t));
                *(header->serveChainSince) = rlpDecodeItemUInt64(coder, keyPairs[1], 1);
            }else if (strcmp(key, "serveStateSince") == 0) {
                header->serveStateSince = malloc(sizeof(uint64_t));
                *(header->serveStateSince) = rlpDecodeItemUInt64(coder, keyPairs[1], 1);
            }else if (strcmp(key, "txRelay") == 0) {
                header->txRelay = ETHEREUM_BOOLEAN_TRUE;
            }else if (strcmp(key, "flowControl/BL") == 0) {
                 header->flowControlBL = malloc(sizeof(uint64_t));
                *(header->flowControlBL) = rlpDecodeItemUInt64(coder, keyPairs[1], 1);
            }else if (strcmp(key, "flowControl/MRC") == 0) {
                 //header->flowControlMRR = malloc(sizeof(uint64_t));
                 size_t mrrItemsCount  = 0;
                 const BRRlpItem* mrrItems = rlpDecodeList(coder, keyPairs[1], &mrrItemsCount);
                 BREthereumLESMRC* mrcs = NULL;
                 if(mrrItemsCount > 0){
                     mrcs = (BREthereumLESMRC*)malloc(sizeof(BREthereumLESMRC) * mrrItemsCount);
                     for(int mrrIdx = 0; mrrIdx < mrrItemsCount; ++mrrIdx){
                            size_t mrrElementsCount  = 0;
                            const BRRlpItem* mrrElements = rlpDecodeList(coder, mrrItems[mrrIdx], &mrrElementsCount);
                            mrcs[mrrIdx].msgCode =  rlpDecodeItemUInt64(coder, mrrElements[0], 1);
                            mrcs[mrrIdx].baseCost =  rlpDecodeItemUInt64(coder, mrrElements[1], 1);
                            mrcs[mrrIdx].reqCost =  rlpDecodeItemUInt64(coder, mrrElements[2], 1);
                     }
                 }
                header->flowControlMRCCount = malloc (sizeof (size_t));
                *header->flowControlMRCCount = mrrItemsCount;
                 header->flowControlMRC = mrcs;
            }else if (strcmp(key, "flowControl/MRR") == 0) {
                 header->flowControlMRR = malloc(sizeof(uint64_t));
                *(header->flowControlMRR) = rlpDecodeItemUInt64(coder, keyPairs[1], 1);
            }
            free (key);
        }else {
            return BRE_LES_CODER_INVALID_STATUS_KEY_PAIR;
        }
    }
    return BRE_LES_CODER_SUCCESS;
}
BREthereumLESDecodeStatus coderDecodeStatus(uint8_t*rlpBytes, size_t rlpBytesSize, BREthereumLESStatusMessage* status) {
 
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpData frameData = {rlpBytesSize, rlpBytes};
    BRRlpItem item = rlpGetItem (coder, frameData);
    
    //Set default values for optional status values
    status->serveHeaders = ETHEREUM_BOOLEAN_FALSE;
    status->flowControlBL = NULL;
    status->flowControlMRC = NULL;
    status->flowControlMRCCount = NULL;
    status->flowControlMRR = NULL;
    status->txRelay = ETHEREUM_BOOLEAN_FALSE;
    status->serveChainSince = NULL;
    status->serveStateSince = NULL;
    
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    
    BREthereumLESDecodeStatus retStatus = _decodeStatus(coder, items, itemsCount, status);

    rlpCoderRelease(coder);
    
    return retStatus;
 
}

extern const char *
lesMessageGetName (BREthereumLESMessageId id) {
    switch (id) {
        case BRE_LES_ID_ANNOUNCE: return "Announce";
        case BRE_LES_ID_STATUS: return "Status";
        case BRE_LES_ID_SEND_TX2: return "Send Tx2";
        case BRE_LES_ID_SEND_TX: return "Send Tx";
        case BRE_LES_ID_GET_TX_STATUS: return "Get Tx Status";
        case BRE_LES_ID_TX_STATUS: return "Tx Status";
        case BRE_LES_ID_GET_BLOCK_BODIES: return "Get Block Bodies";
        case BRE_LES_ID_BLOCK_BODIES: return "Block Bodies";
        case BRE_LES_ID_GET_BLOCK_HEADERS: return "Get Block Headers";
        case BRE_LES_ID_BLOCK_HEADERS: return "Block Headers";
        case BRE_LES_ID_GET_RECEIPTS: return "Get Receipts";
        case BRE_LES_ID_RECEIPTS: return "Receipts";
        case BRE_LES_ID_GET_PROOFS_V2: return "Get Proofs V2";
        case BRE_LES_ID_PROOFS_V2: return "Proofs V2";
        default: return NULL;
    }
}


extern void
statusMessageLogFlowControl (BREthereumLESStatusMessage *message) {
    size_t count = *(message->flowControlMRCCount);
    eth_log (ETH_LOG_TOPIC, "FlowControl/MCC%s", "");
    for (size_t index = 0; index < count; index++) {
        const char *label = lesMessageGetName ((BREthereumLESMessageId) message->flowControlMRC[index].msgCode);
        if (NULL != label) {
            eth_log (ETH_LOG_TOPIC, "=== %d", (BREthereumLESMessageId) message->flowControlMRC[index].msgCode);
            eth_log (ETH_LOG_TOPIC, "    Request : %s", label);
            eth_log (ETH_LOG_TOPIC, "    BaseCost: %llu", message->flowControlMRC[index].baseCost);
            eth_log (ETH_LOG_TOPIC, "    ReqCost : %llu", message->flowControlMRC[index].reqCost);
        }
    }
}

//
//  Header synchronisation
//
void coderAnnounce(UInt256 headHash, uint64_t headNumber, uint64_t headTd, uint64_t reorgDepth, size_t flowControlMRRCount,
                               BREthereumLESAnnounceRequest* handshakeVals, size_t handshakeValsCount,
                               uint8_t**rlpBytes, size_t* rlpBytesSize) {
    
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem* items = (BRRlpItem*)malloc(sizeof(BRRlpItem)* (handshakeValsCount + 5));
    int idx = 0;
    
    items[idx++] = rlpEncodeItemUInt64(coder, 0x01,1);
    items[idx++] = rlpEncodeItemBytes(coder, headHash.u8, sizeof(headHash.u8));
    items[idx++] = rlpEncodeItemUInt64(coder, headNumber,1);
    items[idx++] = rlpEncodeItemUInt64(coder, headTd,1);
    items[idx++] = rlpEncodeItemUInt64(coder, reorgDepth,1);
    
    for(int i = 0; i < handshakeValsCount; ++i){
        BREthereumLESAnnounceRequest* keyPair = &handshakeVals[i];
        BRRlpItem keyPairItem[2];
        keyPairItem[0] = rlpEncodeItemString(coder, keyPair->key);
        keyPairItem[1] = rlpEncodeItemString(coder, keyPair->key);
        if (strcmp(keyPair->key, "txRelay") == 0 || strcmp(keyPair->key, "serveHeaders") == 0) {
            items[idx++] = rlpEncodeListItems(coder, keyPairItem, 1);
        }else {
            items[idx++] = rlpEncodeListItems(coder, keyPairItem, 2);
        }
    }
    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    rlpDataExtract(coder, encoding, rlpBytes, rlpBytesSize);
    rlpCoderRelease(coder);
    free(items);
}
BREthereumLESDecodeStatus coderDecodeAnnounce(uint8_t*rlpBytes, size_t rlpBytesSize,
                                                    BREthereumHash* hash,
                                                    uint64_t* headNumber, UInt256* headTd, uint64_t* reorgDepth,
                                                    BREthereumLESStatusMessage* status) {
    
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpData data = {rlpBytesSize, rlpBytes};
    BRRlpItem item = rlpGetItem (coder, data);
    
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
   
   // [+0x01, headHash: B_32, headNumber: P, headTd: P, reorgDepth: P, [key_0, value_0], [key_1, value_1], ...]
    BRRlpData hashData = rlpDecodeItemBytes(coder, items[0]);
    memcpy(hash->bytes, hashData.bytes, hashData.bytesCount);
    rlpDataRelease(hashData);
    
    *headNumber = rlpDecodeItemUInt64(coder, items[1], 1);
    *headTd = rlpDecodeItemUInt256(coder, items[2], 1);
    *reorgDepth = rlpDecodeItemUInt64(coder, items[3], 1);
    
    size_t keyValuesCount = itemsCount - 4;
    
    for(int i = 0; i < keyValuesCount; ++i){
         _decodeStatus(coder, &items[4 + i], 1, status);
    }
    rlpCoderRelease(coder);
    
    return BRE_LES_CODER_SUCCESS;
}
extern BREthereumLESDecodeStatus coderDecodeStatus(uint8_t*rlpBytes, size_t rlpBytesSize, BREthereumLESStatusMessage* status);

BRRlpData coderGetBlockHeaders(uint64_t message_id_offset,
                                      uint64_t reqId,
                                      uint64_t block,
                                      uint64_t maxHeaders,
                                      uint64_t skip,
                                      uint64_t reverse) {
    
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem items[3];
    BRRlpItem blockItems[4];
    int idx = 0;
    
    items[idx++] = rlpEncodeItemUInt64(coder, reqId,1);

    blockItems[0] = rlpEncodeItemUInt64(coder, block, 1);
    blockItems[1] = rlpEncodeItemUInt64(coder, maxHeaders, 1);
    blockItems[2] = rlpEncodeItemUInt64(coder, skip, 1);
    blockItems[3] = rlpEncodeItemUInt64(coder, reverse, 1);
   
    items[idx++] = rlpEncodeListItems(coder, blockItems, 4);
    
    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    
    BRRlpData messageListData;
    
    rlpDataExtract(coder, encoding, &messageListData.bytes, &messageListData.bytesCount);
    
    BRRlpData retData = _encodePayloadId(coder, messageListData, message_id_offset + BRE_LES_ID_GET_BLOCK_HEADERS);
    
    rlpCoderRelease(coder);
    rlpDataRelease(messageListData);

    return retData;
}

BREthereumLESDecodeStatus coderDecodeBlockHeaders(uint8_t*rlpBytes, size_t rlpBytesSize,  uint64_t* reqId, uint64_t* bv,
                                   BREthereumBlockHeader** blockHeaders) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpData frameData = {rlpBytesSize, rlpBytes};
    BRRlpItem item = rlpGetItem (coder, frameData);
    
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    *reqId = rlpDecodeItemUInt64(coder, items[0], 1);
    *bv = rlpDecodeItemUInt64(coder, items[1], 1);
    
    size_t blocksCount = 0;
    const BRRlpItem *blocks = rlpDecodeList(coder, items[2], &blocksCount);
    BREthereumBlockHeader*headers;
    
    array_new(headers, blocksCount);
    
    for(int i = 0; i < blocksCount; ++i){
        array_add(headers, blockHeaderRlpDecode(blocks[i], RLP_TYPE_NETWORK, coder));
    }
    rlpCoderRelease(coder);
    
    *blockHeaders = headers;
    
    return BRE_LES_CODER_SUCCESS;
}

void coderBlockHeaders(uint64_t reqId, uint64_t bv, const BREthereumBlockHeader* blockHeader,  uint8_t**rlpBytes, size_t* rlpByesSize) {
}

//
// On-demand data retrieval
//
BRRlpData coderGetBlockBodies(uint64_t message_id_offset, uint64_t reqId, BREthereumHash* blockHashes) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem items[2];
    BRRlpItem blockItems[array_count(blockHashes)];
    int idx = 0;
    
    items[idx++] = rlpEncodeItemUInt64(coder, reqId,1);

    // [+0x04, reqID: P, [hash_0: B_32, hash_1: B_32, ...]]

    for(int i = 0; i < array_count(blockHashes); ++i){
        blockItems[i] = rlpEncodeItemBytes(coder, blockHashes[i].bytes, 32);
    }
    items[idx++] = rlpEncodeListItems(coder, blockItems, array_count(blockHashes));
    
    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    
    BRRlpData messageListData;
    
    rlpDataExtract(coder, encoding, &messageListData.bytes, &messageListData.bytesCount);
    
    BRRlpData retData = _encodePayloadId(coder, messageListData, message_id_offset + BRE_LES_ID_GET_BLOCK_BODIES);
    
    rlpCoderRelease(coder);
    rlpDataRelease(messageListData);

    return retData;

}
BRRlpData coderGetProofsV2(uint64_t message_id_offset, uint64_t reqId, BREthereumLESProofsRequest* proofs) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem items[2];
    BRRlpItem blockItems[array_count(proofs)];
    int idx = 0;
    
    // items[idx++] = rlpEncodeItemUInt64(coder, 0x02,1);
    items[idx++] = rlpEncodeItemUInt64(coder, reqId,1);

    // [+0x08, reqID: P, [ [blockhash: B_32, key: B_32, key2: B_32, fromLevel: P], ...]]
    for(int i = 0; i < array_count(proofs); ++i){
        BRRlpItem proofItems[4];
        proofItems[0] = rlpEncodeItemBytes(coder, proofs[i].blockHash.bytes, 32);
        proofItems[1] = rlpEncodeItemBytes(coder, proofs[i].key.bytes, 32);
        proofItems[2] = rlpEncodeItemBytes(coder, proofs[i].key2.bytes, 32);
        proofItems[3] = rlpEncodeItemUInt64(coder, proofs[i].fromLevel,1);
        blockItems[i] = rlpEncodeListItems(coder, proofItems, 4);
    }
    
    items[idx++] = rlpEncodeListItems(coder, blockItems, array_count(proofs));
    
    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    
    BRRlpData messageListData;
    
    rlpDataExtract(coder, encoding, &messageListData.bytes, &messageListData.bytesCount);
    
    BRRlpData retData = _encodePayloadId(coder, messageListData, message_id_offset + BRE_LES_ID_GET_PROOFS_V2);
    
    rlpCoderRelease(coder);
    rlpDataRelease(messageListData);

    return retData;
}
BREthereumLESDecodeStatus coderDecodeBlockBodies(uint8_t*rlpBytes, size_t rlpBytesSize, uint64_t* reqId, uint64_t* bv, BREthereumNetwork network, BREthereumBlockHeader***ommers,  BREthereumTransaction***transactions) {

 // [+0x05, reqID: P, BV: P, [ [transactions_0, uncles_0] , ...]]
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpData frameData = {rlpBytesSize, rlpBytes};
    BRRlpItem item = rlpGetItem (coder, frameData);
    
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    *reqId = rlpDecodeItemUInt64(coder, items[0], 1);
    *bv = rlpDecodeItemUInt64(coder, items[1], 1);
    
    size_t txtsOmmersCount = 0;
    const BRRlpItem *txtsOmmersItems = rlpDecodeList(coder, items[2], &txtsOmmersCount);
    BREthereumBlockHeader** ommersHeaders;
    BREthereumTransaction** actualTransactions;
    
    array_new(ommersHeaders, txtsOmmersCount);
    array_new(actualTransactions, txtsOmmersCount);

    for(int i = 0; i < txtsOmmersCount; ++i){
        size_t txtsOmmersDataCount = 0;
        const BRRlpItem *txtsOmmersDataItems = rlpDecodeList(coder, txtsOmmersItems[i], &txtsOmmersDataCount);
        
        if(txtsOmmersDataCount != 2){
            return BRE_LES_CODER_UNABLE_TO_DECODE_ERROR;
        }
        
        array_add(actualTransactions,blockTransactionsRlpDecode(txtsOmmersDataItems[0],network,RLP_TYPE_NETWORK,coder));
        array_add(ommersHeaders, blockOmmersRlpDecode(txtsOmmersDataItems[1],network,RLP_TYPE_NETWORK,coder));
    }
 
    rlpCoderRelease(coder);
    
    *ommers = ommersHeaders;
    *transactions = actualTransactions;
    
    return BRE_LES_CODER_SUCCESS;
}

BRRlpData coderGetReceipts(uint64_t message_id_offset, uint64_t reqId, BREthereumHash* blockHashes) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem items[2];
    BRRlpItem blockItems[array_count(blockHashes)];
    int idx = 0;
    
   // items[idx++] = rlpEncodeItemUInt64(coder, 0x02,1);
    items[idx++] = rlpEncodeItemUInt64(coder, reqId,1);

    // [+0x04, reqID: P, [hash_0: B_32, hash_1: B_32, ...]]

    for(int i = 0; i < array_count(blockHashes); ++i){
        blockItems[i] = rlpEncodeItemBytes(coder, blockHashes[i].bytes, 32);
    }
    items[idx++] = rlpEncodeListItems(coder, blockItems, array_count(blockHashes));
    
    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    
    BRRlpData messageListData;
    
    rlpDataExtract(coder, encoding, &messageListData.bytes, &messageListData.bytesCount);
    
    BRRlpData retData = _encodePayloadId(coder, messageListData, message_id_offset + BRE_LES_ID_GET_RECEIPTS);
    
    rlpCoderRelease(coder);
    rlpDataRelease(messageListData);

    return retData;

}
BREthereumLESDecodeStatus coderDecodeReceipts(uint8_t*rlpBytes, size_t rlpBytesSize, uint64_t* reqId, uint64_t* bv, BREthereumTransactionReceipt***receipts) {

    // [+0x07, reqID: P, BV: P, [ [receipt_0, receipt_1, ...], ...]]
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpData frameData = {rlpBytesSize, rlpBytes};
    BRRlpItem item = rlpGetItem (coder, frameData);
    
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    *reqId = rlpDecodeItemUInt64(coder, items[0], 1);
    *bv = rlpDecodeItemUInt64(coder, items[1], 1);
    
    size_t receiptsCount = 0;
    const BRRlpItem *receiptItems = rlpDecodeList(coder, items[2], &receiptsCount);
    BREthereumTransactionReceipt**actualReceipts;
    
    array_new(actualReceipts, receiptsCount);

    for(int i = 0; i < receiptsCount; ++i){
        BREthereumTransactionReceipt*receiptData;
        size_t blockReceiptsCount = 0;
        const BRRlpItem *blockReceiptItems = rlpDecodeList(coder, receiptItems[i], &blockReceiptsCount);
        array_new(receiptData,blockReceiptsCount);
        for(int j = 0; j < blockReceiptsCount; ++j){
            array_add(receiptData, transactionReceiptRlpDecode(blockReceiptItems[j], coder));
        }
        array_add(actualReceipts, receiptData);
    }
    rlpCoderRelease(coder);
    *receipts = actualReceipts;
    return BRE_LES_CODER_SUCCESS;
}
//
// Transaction relaying and status retrieval
//
static BRRlpData _encodeTxts(uint64_t msgId, uint64_t message_id_offset, uint64_t reqId, BREthereumTransaction transactions[], BREthereumNetwork network, BREthereumRlpType type) {

    size_t transactionsCount = array_count(transactions);
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem items[2];
    int idx = 0;
    
    items[idx++] = rlpEncodeItemUInt64(coder, reqId,1);

    BRRlpItem txtsItems[transactionsCount];
    
    for(int i = 0; i < transactionsCount; ++i){
        txtsItems[i] = transactionRlpEncode(transactions[i], network, type, coder);
    }
    items[idx++] = rlpEncodeListItems(coder, txtsItems, transactionsCount);

    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    
    BRRlpData messageListData;
    rlpDataExtract(coder, encoding, &messageListData.bytes, &messageListData.bytesCount);
    
    BRRlpData retData = _encodePayloadId(coder, messageListData, message_id_offset + msgId);

    rlpCoderRelease(coder);
    
    return retData;
}

BRRlpData coderSendTx(uint64_t message_id_offset, uint64_t reqId, BREthereumTransaction transactions[], BREthereumNetwork network, BREthereumRlpType type) {
    
    return _encodeTxts(BRE_LES_ID_SEND_TX, message_id_offset, reqId, transactions,network,type);
}
BRRlpData coderSendTxV2(uint64_t message_id_offset, uint64_t reqId, BREthereumTransaction transactions[], BREthereumNetwork network, BREthereumRlpType type) {

    return _encodeTxts(BRE_LES_ID_SEND_TX2,message_id_offset, reqId, transactions,network,type);
}
BRRlpData coderGetTxStatus(uint64_t message_id_offset, uint64_t reqId, BREthereumHash* transactions) {

    BRRlpCoder coder = rlpCoderCreate();
    size_t transactionsCount = array_count(transactions);
    BRRlpItem items[transactionsCount + 2];
    int idx = 0;
    
  //  items[idx++] = rlpEncodeItemUInt64(coder, BRE_LES_ID_GET_TX_STATUS,1);
    items[idx++] = rlpEncodeItemUInt64(coder, reqId,1);

    BRRlpItem txtsItems[transactionsCount];
    for(int i = 0; i < transactionsCount; ++i){
        txtsItems[i] = rlpEncodeItemBytes(coder, transactions[i].bytes, sizeof(transactions[i].bytes));
    }
    items[idx++] = rlpEncodeListItems(coder, txtsItems, transactionsCount);

    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    
    BRRlpData messageListData;
    rlpDataExtract(coder, encoding, &messageListData.bytes, &messageListData.bytesCount);
    
    BRRlpData retData = _encodePayloadId(coder, messageListData, message_id_offset + BRE_LES_ID_GET_TX_STATUS);

    rlpDataRelease(messageListData);
    rlpCoderRelease(coder);
    
    return retData;
}
BREthereumLESDecodeStatus coderDecodeTxStatus(uint8_t*rlpBytes, size_t rlpBytesSize, uint64_t* reqId, uint64_t* bv, BREthereumTransactionStatus** replies, size_t* repliesCount){

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpData data = {rlpBytesSize, rlpBytes};

    rlpShow(data, "TxtStatus");

    BRRlpItem item = rlpGetItem (coder, data);
  
    //Set default values for optional status values
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    *reqId = rlpDecodeItemUInt64(coder, items[0],1);
    *bv = rlpDecodeItemUInt64(coder, items[1],1);
    size_t statusesCount;
    const BRRlpItem *statuses = rlpDecodeList(coder, items[2], &statusesCount);
    
    BREthereumTransactionStatus*retReplies = (BREthereumTransactionStatus*)malloc(sizeof(BREthereumTransactionStatus) * statusesCount);
    for(int i = 0; i < statusesCount; ++i){
        retReplies[i] = transactionStatusRLPDecode(statuses[i], coder);
    }
    rlpCoderRelease(coder);

    *replies = retReplies;
    *repliesCount = statusesCount;

    return BRE_LES_CODER_SUCCESS;

}
/* 
void ethereumLESTxStatus( uint64_t reqId, uint64_t bv, BREthereumTransactionStatusLES* replies, size_t repliesCount, uint8_t**rlpBytes, size_t* rlpBytesSize){

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem* items = (BRRlpItem*)malloc(sizeof(BRRlpItem)* (repliesCount + 3));
    int idx = 0;
    
    items[idx++] = rlpEncodeItemUInt64(coder, 0x15,0);
    items[idx++] = rlpEncodeItemUInt64(coder, reqId,0);
    items[idx++] = rlpEncodeItemUInt64(coder, bv,0);
    
    BRRlpItem* txtsItems = (BRRlpItem*)malloc(sizeof(BRRlpItem)* repliesCount);
    
    for(int i = 0; i < repliesCount; ++i){
        BRRlpItem statuses[2];
        size_t size = 1;
        statuses[0] = rlpEncodeItemUInt64(coder, replies[i].status, 0);
        if(replies[i].status == TXSTATUS_INCLUDED) {
            BRRlpItem includedData[3];
            includedData[0] = rlpEncodeItemBytes(coder, replies[i].u.included_data.blockHash, 32);
            includedData[1] = rlpEncodeItemUInt64(coder, replies[i].u.included_data.blockNumber, 0);
            includedData[2] = rlpEncodeItemUInt64(coder, replies[i].u.included_data.txIndex, 0);
            statuses[1] = rlpEncodeListItems(coder, includedData, 3);
            size = 2;
        }else if(replies[i].status == TXSTATUS_ERROR)  {
            statuses[1] = rlpEncodeItemString(coder, replies[i].u.error_message);
            size = 2;
        }
        txtsItems[i] = rlpEncodeListItems(coder, statuses, size);
    }
    items[idx++] = rlpEncodeListItems(coder, txtsItems, repliesCount);

    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    rlpDataExtract(coder, encoding, rlpBytes, rlpBytesSize);
    rlpCoderRelease(coder);
    free(items);
    free(txtsItems);
}
*/ 
