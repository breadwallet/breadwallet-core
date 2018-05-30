//
//  BREthereumLESCoder.c
//  breadwallet
//
//  Created by Lamont Samuels on 5/29/18.
//  Copyright © 2018 breadwallet LLC. All rights reserved.
//

#include "BREthereumLESCoder.h"


#define TXSTATUS_INCLUDED 3
#define TXSTATUS_ERROR 4

typedef enum {
    BRE_LES_SUCCESS=0,
    BRE_LES_INVALID_MSG_ID_ERROR,
    BRE_LES_INVALID_STATUS_KEY_PAIR,
    BRE_LES_UNABLE_TO_DECODE_ERROR,
}BREthereumLESDecodeStatus;

typedef struct {
    uint64_t msgCode;
    uint64_t baseCost;
    uint64_t reqCost;
}BREthereumLESMRC;


//
// LES Status Structures
//
typedef struct {
    uint64_t protocolVersion;
    uint64_t chainId;
    uint64_t headerTd;
    uint8_t headHash[32];
    uint64_t headNum;
    uint8_t genesisHash[32];
    // Note: The below fields are optional LPV1
    BREthereumBoolean serveHeaders;
    uint64_t* serveChainSince;
    uint64_t* serveStateSince;
    BREthereumBoolean txRelay;
    uint64_t*flowControlBL;
    BREthereumLESMRC*flowControlMRC;
    size_t* flowControlMRCCount;
    uint64_t*flowControlMRR;
    uint64_t announceType;
}BREthereumLESStatus;

//
// LES Reply Structures
//
typedef struct {
    UInt256* transaction;
    UInt256* uncle;
}BREthereumBlockBody;

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

typedef struct {
    uint64_t status;
    union {
        //Included data
        struct {
           uint8_t  blockHash[32];
           uint64_t blockNumber;
           uint64_t txIndex;
        }included_data;
        char* error_message;
    } u;
}BREthereumTransactionStatusReply;


//
// LES Request Structures
//
typedef struct {
    char* key;
    void* value;
}BREthereumAnnounceRequest;


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

//
// Handshake messages
//
static void _encodeKeyValueStatus(BRRlpCoder coder, BRRlpItem* keyPair, char* key, void* value, size_t auxValueCount) {

    if(strcmp(key, "protocolVersion") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "protocolVersion");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),0);
    }else if (strcmp(key, "networkID") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "networkId");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),0);
    }else if (strcmp(key, "headTd") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "headTd");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),0);
    }else if (strcmp(key, "headHash") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "headHash");
        keyPair[1] = rlpEncodeItemBytes(coder, ((uint8_t *)value), 32);
    }else if (strcmp(key, "headNum") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "headNum");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),0);
    }else if (strcmp(key, "genesisHash") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "genesisHash");
        keyPair[1] = rlpEncodeItemBytes(coder, ((uint8_t *)value), 32);
    }else if (strcmp(key, "serveHeaders") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "serveHeaders");
    }else if (strcmp(key, "serveChainSince") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "serveChainSince");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),0);
    }else if (strcmp(key, "serveStateSince") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "serveStateSince");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),0);
    }else if (strcmp(key, "txRelay") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "txRelay");
    }else if (strcmp(key, "flowControl/BL") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/BL");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),0);
    }else if (strcmp(key, "flowControl/MRR") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/MRC");
        size_t count = auxValueCount;
        BRRlpItem mrcItems[count];
        for(int idx = 0; idx < count; ++idx){
            BRRlpItem mrcElements [3];
            BREthereumLESMRC* flowControlMRC = ((BREthereumLESMRC *)value);
            mrcElements[0] = rlpEncodeItemUInt64(coder,flowControlMRC[idx].msgCode,0);
            mrcElements[1] = rlpEncodeItemUInt64(coder,flowControlMRC[idx].baseCost,0);
            mrcElements[2] = rlpEncodeItemUInt64(coder,flowControlMRC[idx].reqCost,0);
            mrcItems[idx] = rlpEncodeListItems(coder, mrcElements, 3);
        }
        keyPair[1] = rlpEncodeListItems(coder, mrcItems, count);
    }else if (strcmp(key, "flowControl/MRR") == 0) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/MRR");
        keyPair[1] = rlpEncodeItemUInt64(coder, *((uint64_t *)value),0);
    }
}

static void _encodeStatus(BREthereumLESStatus* status, BRRlpCoder coder, BRRlpItem* statusItems, int* ioIdx) {

    int curIdx = *ioIdx;

    //protocolVersion
    BRRlpItem keyPair [2];
    keyPair[0] = rlpEncodeItemString(coder, "protocolVersion");
    keyPair[1] = rlpEncodeItemUInt64(coder, status->protocolVersion,0);
    statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    
    //networkId
    keyPair[0] = rlpEncodeItemString(coder, "networkId");
    keyPair[1] = rlpEncodeItemUInt64(coder, status->chainId,0);
    statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);

    //headTd
    keyPair[0] = rlpEncodeItemString(coder, "headTd");
    keyPair[1] = rlpEncodeItemUInt64(coder, status->headerTd,0);
    statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
 
    //headHash
    keyPair[0] = rlpEncodeItemString(coder, "headHash");
    keyPair[1] = rlpEncodeItemBytes(coder, status->headHash, sizeof(status->headHash));
    statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
 
    //headNum
    keyPair[0] = rlpEncodeItemString(coder, "headNum");
    keyPair[1] = rlpEncodeItemUInt64(coder, status->headNum,0);
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
        keyPair[1] = rlpEncodeItemUInt64(coder, *(status->serveChainSince),0);
        statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    
    //serveStateSince
    if(status->serveStateSince != NULL) {
        keyPair[0] = rlpEncodeItemString(coder, "serveStateSince");
        keyPair[1] = rlpEncodeItemUInt64(coder, *(status->serveStateSince),0);
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
        keyPair[1] = rlpEncodeItemUInt64(coder, *(status->flowControlBL),0);
        statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    //flowControl/MRC
    if(status->flowControlBL != NULL) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/MRC");
        size_t count = *(status->flowControlMRCCount);
        BRRlpItem mrcItems[count];
        for(int idx = 0; idx < count; ++idx){
            BRRlpItem mrcElements [3];
            mrcElements[0] = rlpEncodeItemUInt64(coder,status->flowControlMRC[idx].msgCode,0);
            mrcElements[1] = rlpEncodeItemUInt64(coder,status->flowControlMRC[idx].baseCost,0);
            mrcElements[2] = rlpEncodeItemUInt64(coder,status->flowControlMRC[idx].reqCost,0);
            mrcItems[idx] = rlpEncodeListItems(coder, mrcElements, 3);
        }
        keyPair[1] = rlpEncodeListItems(coder, mrcItems, count);
        statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    //flowControl/MRR
    if(status->flowControlMRR != NULL) {
        keyPair[0] = rlpEncodeItemString(coder, "flowControl/MRR");
        keyPair[1] = rlpEncodeItemUInt64(coder, *(status->flowControlMRR),0);
        statusItems[curIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    *ioIdx = curIdx;
}
void ethereumLESEncodeStatus(BREthereumLESStatus* status, uint8_t**rlpBytes, size_t* rlpBytesSize) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem statusItems[15];
    int ioIdx = 0;
    
    statusItems[ioIdx++] = rlpEncodeItemUInt64(coder, 0x00,0);
    _encodeStatus(status,coder,statusItems,&ioIdx);
    
    //announceType
    if(status->protocolVersion == 0x02){
        BRRlpItem keyPair [2];
        keyPair[0] = rlpEncodeItemString(coder, "announceType");
        keyPair[1] = rlpEncodeItemUInt64(coder, status->announceType,0);
        statusItems[ioIdx++] = rlpEncodeListItems(coder, keyPair, 2);
    }
    BRRlpItem encoding = rlpEncodeListItems(coder, statusItems, ioIdx);
    rlpDataExtract(coder, encoding, rlpBytes, rlpBytesSize);
    rlpCoderRelease(coder);
}
static BREthereumLESDecodeStatus _decodeStatus(BRRlpCoder coder, const BRRlpItem *items, size_t itemsCount, BREthereumLESStatus* header){

    uint64_t messageId = rlpDecodeItemUInt64(coder, items[0],0);
    if(messageId != 0x00){
        return BRE_LES_INVALID_MSG_ID_ERROR;
    }
    for(int i= 1; i < itemsCount; ++i) {
        size_t keyPairCount;
        const BRRlpItem *keyPairs = rlpDecodeList(coder, items[i], &keyPairCount);
        if(keyPairCount > 0){
            char * key = rlpDecodeItemString(coder, keyPairs[0]);
            if(strcmp(key, "protocolVersion") == 0) {
                header->protocolVersion = rlpDecodeItemUInt64(coder, keyPairs[1], 0);
            }else if (strcmp(key, "networkID") == 0) {
                header->chainId = rlpDecodeItemUInt64(coder, keyPairs[1], 0);
            }else if (strcmp(key, "headTd") == 0) {
                header->headerTd = rlpDecodeItemUInt64(coder, keyPairs[1], 0);
            }else if (strcmp(key, "headHash") == 0) {
                BRRlpData hashData = rlpDecodeItemBytes(coder, keyPairs[1]);
                memcpy(header->headHash, hashData.bytes, hashData.bytesCount);
                rlpDataRelease(hashData);
            }else if (strcmp(key, "announceType") == 0) {
                header->announceType = rlpDecodeItemUInt64(coder, keyPairs[1], 0);
            }else if (strcmp(key, "headNum") == 0) {
                header->headerTd = rlpDecodeItemUInt64(coder, keyPairs[1], 0);
            }else if (strcmp(key, "genesisHash") == 0) {
                BRRlpData hashData = rlpDecodeItemBytes(coder, keyPairs[1]);
                memcpy(header->genesisHash, hashData.bytes, hashData.bytesCount);
                rlpDataRelease(hashData);
            }else if (strcmp(key, "serveHeaders") == 0) {
                header->serveHeaders = ETHEREUM_BOOLEAN_TRUE;
            }else if (strcmp(key, "serveChainSince") == 0) {
                header->serveChainSince = malloc(sizeof(uint64_t));
                *(header->serveChainSince) = rlpDecodeItemUInt64(coder, keyPairs[1], 0);
            }else if (strcmp(key, "serveStateSince") == 0) {
                header->serveStateSince = malloc(sizeof(uint64_t));
                *(header->serveStateSince) = rlpDecodeItemUInt64(coder, keyPairs[1], 0);
            }else if (strcmp(key, "txRelay") == 0) {
                header->txRelay = ETHEREUM_BOOLEAN_TRUE;
            }else if (strcmp(key, "flowControl/BL") == 0) {
                 header->flowControlBL = malloc(sizeof(uint64_t));
                *(header->flowControlBL) = rlpDecodeItemUInt64(coder, keyPairs[1], 0);
            }else if (strcmp(key, "flowControl/MRR") == 0) {
                 header->flowControlMRR = malloc(sizeof(uint64_t));
                 size_t mrrItemsCount  = 0;
                 const BRRlpItem* mrrItems = rlpDecodeList(coder, keyPairs[1], &mrrItemsCount);
                 BREthereumLESMRC* mrcs = NULL;
                 if(mrrItemsCount > 0){
                     mrcs = (BREthereumLESMRC*)malloc(sizeof(BREthereumLESMRC) * mrrItemsCount);
                     for(int mrrIdx = 0; mrrIdx < mrrItemsCount; ++mrrIdx){
                            size_t mrrElementsCount  = 0;
                            const BRRlpItem* mrrElements = rlpDecodeList(coder, mrrItems[mrrIdx], &mrrElementsCount);
                            mrcs[mrrIdx].msgCode =  rlpDecodeItemUInt64(coder, mrrElements[0], 0);
                            mrcs[mrrIdx].baseCost =  rlpDecodeItemUInt64(coder, mrrElements[1], 0);
                            mrcs[mrrIdx].reqCost =  rlpDecodeItemUInt64(coder, mrrElements[2], 0);
                     }
                 }
                 header->flowControlMRC = mrcs;
            }else if (strcmp(key, "flowControl/MRR") == 0) {
                 header->flowControlMRR = malloc(sizeof(uint64_t));
                *(header->flowControlMRR) = rlpDecodeItemUInt64(coder, keyPairs[1], 0);
            }
        }else {
            return BRE_LES_INVALID_STATUS_KEY_PAIR;
        }
    }
    return BRE_LES_SUCCESS;
}
BREthereumLESDecodeStatus ethereumLESDecodeStatus(uint8_t*rlpBytes, size_t rlpBytesSize, BREthereumLESStatus* status) {
 
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
//
//  Header synchronisation
//
void ethereumLESAnnounce(UInt256 headHash, uint64_t headNumber, uint64_t headTd, uint64_t reorgDepth, size_t flowControlMRRCount,
                               BREthereumAnnounceRequest* handshakeVals, size_t handshakeValsCount,
                               uint8_t**rlpBytes, size_t* rlpBytesSize) {
    
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem* items = (BRRlpItem*)malloc(sizeof(BRRlpItem)* (handshakeValsCount + 5));
    int idx = 0;
    
    items[idx++] = rlpEncodeItemUInt64(coder, 0x01,0);
    items[idx++] = rlpEncodeItemBytes(coder, headHash.u8, sizeof(headHash.u8));
    items[idx++] = rlpEncodeItemUInt64(coder, headNumber,0);
    items[idx++] = rlpEncodeItemUInt64(coder, headTd,0);
    items[idx++] = rlpEncodeItemUInt64(coder, reorgDepth,0);
    
    for(int i = 0; i < handshakeValsCount; ++i){
        BREthereumAnnounceRequest* keyPair = &handshakeVals[i];
        BRRlpItem keyPairItem[2];
        keyPairItem[0] = rlpEncodeItemString(coder, keyPair->key);
        keyPairItem[1] = rlpEncodeItemString(coder, keyPair->key);
        _encodeKeyValueStatus(coder, keyPairItem,keyPair->key, keyPair->value, flowControlMRRCount);
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

void  ethereumLESGetBlockHeaders(uint64_t reqId,
                                 BREthereumBlock block,
                                 uint64_t maxHeaders,
                                 uint64_t skip,
                                 uint64_t reverse,
                                 uint8_t**rlpBytes, size_t* rlpBytesSize) {
    
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem items[3];
    BRRlpItem blockItems[4];
    int idx = 0;
    
    items[idx++] = rlpEncodeItemUInt64(coder, 0x02,0);
    items[idx++] = rlpEncodeItemUInt64(coder, reqId,0);

    BREthereumHash bHash = blockGetHash(block);
    blockItems[0] = rlpEncodeItemBytes(coder, bHash.bytes, sizeof(bHash.bytes));
    blockItems[1] = rlpEncodeItemUInt64(coder, maxHeaders, 0);
    blockItems[2] = rlpEncodeItemUInt64(coder, skip, 0);
    blockItems[3] = rlpEncodeItemUInt64(coder, reverse, 0);
   
    items[idx++] = rlpEncodeListItems(coder, blockItems, 4);
    
    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    rlpDataExtract(coder, encoding, rlpBytes, rlpBytesSize);
    rlpCoderRelease(coder);
}

BREthereumLESDecodeStatus ethereumLESDecodeBlockHeaders(uint8_t*rlpBytes, size_t rlpBytesSize,  uint64_t* reqId, uint64_t* bv,
                                   BREthereumBlockHeader** blockHeaders, size_t * blockHeadersCount) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpData frameData = {rlpBytesSize, rlpBytes};
    BRRlpItem item = rlpGetItem (coder, frameData);
    
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    uint64_t msgId = rlpDecodeItemUInt64(coder, items[0], 0);
    if(msgId != 0x03) {
        return BRE_LES_INVALID_MSG_ID_ERROR;
    }
    *reqId = rlpDecodeItemUInt64(coder, items[1], 0);
    *bv = rlpDecodeItemUInt64(coder, items[2], 0);
    
    size_t blocksCount = 0;
    const BRRlpItem *blocks = rlpDecodeList(coder, items[3], &blocksCount);
    BREthereumBlockHeader*headers = (BREthereumBlockHeader*)malloc(sizeof(BREthereumBlockHeader) * blocksCount);
    
    for(int i = 0; i < blocksCount; ++i){
        BRRlpData data;
        rlpDataExtract(coder, blocks[i], &data.bytes, &data.bytesCount);
        headers[i] = blockHeaderDecodeRLP(data);
        rlpDataRelease(data);
    }
    rlpCoderRelease(coder);
    
    *blockHeaders = headers;
    *blockHeadersCount = blocksCount;
    
    return BRE_LES_SUCCESS;
}

void ethereumLESBlockHeaders(uint64_t reqId, uint64_t bv, const BREthereumBlockHeader* blockHeader,  uint8_t**rlpBytes, size_t* rlpByesSize) {



}
//
// On-demand data retrieval
//
void ethereumLESGetBlockBodies(UInt256* blockHashes, size_t blockHashesCount, uint8_t**rlpBytes, size_t* rlpByesSize) {

       //TODO: Decode the rlp header from EthereumBlock.h once implemented


}

void ethereumLESDecodeBlockBodies(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv, BREthereumBlockBody** blockBodies, size_t* blockBodiesCount) {

       //TODO: Decode the rlp header from EthereumBlock.h once implemented

}

void ethereumLESBlockBodies(uint64_t reqId, uint64_t bv, const BREthereumBlockBody* blockBodies, size_t blockBoidesCount, uint8_t**rlpBytes, size_t* rlpByesSize) {

       //TODO: Decode the rlp header from EthereumBlock.h once implemented

}
void ethereumLESGetReceipts(uint64_t reqId, UInt256* receipts, size_t receiptsCount, uint8_t**rlpBytes, size_t* rlpByesSize) {



}
void ethereumLESDecodeReceipts(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv, BREthereumReceipt**receipts, size_t* receiptsCount) {


}

void ethereumLESReceipts(uint64_t reqId, uint64_t bv, BREthereumReceipt* receipts, size_t receiptsCount, uint8_t**rlpBytes, size_t* rlpByesSize) {


}
//
// Transaction relaying and status retrieval
//
static void _encodeTxts(uint64_t msgId, uint64_t reqId, BREthereumTransaction* transactions, size_t transactionsCount, BREthereumNetwork network, BREthereumTransactionRLPType type,  uint8_t**rlpBytes, size_t* rlpBytesSize) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem* items = (BRRlpItem*)malloc(sizeof(BRRlpItem)* (transactionsCount + 1));
    int idx = 0;
    
    items[idx++] = rlpEncodeItemUInt64(coder, msgId,0);
    items[idx++] = rlpEncodeItemUInt64(coder, reqId,0);

    BRRlpItem* txtsItems = (BRRlpItem*)malloc(sizeof(BRRlpItem)* transactionsCount);
    for(int i = 0; i < transactionsCount; ++i){
        BRRlpData data = transactionEncodeRLP(transactions[i], network,type);
        txtsItems[i] = rlpEncodeItemBytes(coder, data.bytes, data.bytesCount);
        rlpDataRelease(data);
    }
    items[idx++] = rlpEncodeListItems(coder, txtsItems, transactionsCount);

    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    rlpDataExtract(coder, encoding, rlpBytes, rlpBytesSize);
    rlpCoderRelease(coder);
    free(items);
    free(txtsItems);
}

void ethereumLESSendTxt(uint64_t reqId, BREthereumTransaction* transactions, size_t transactionsCount, BREthereumNetwork network, BREthereumTransactionRLPType type,  uint8_t**rlpBytes, size_t* rlpBytesSize) {
    
    _encodeTxts(0x0c,reqId, transactions,transactionsCount,network,type,rlpBytes,rlpBytesSize);
}
void ethereumLESSendTxtV2(uint64_t reqId, BREthereumTransaction* transactions, size_t transactionsCount, BREthereumNetwork network, BREthereumTransactionRLPType type, uint8_t**rlpBytes, size_t* rlpBytesSize) {

    _encodeTxts(0x13,reqId, transactions,transactionsCount,network,type,rlpBytes,rlpBytesSize);
}
void ethereumLESGetTxStatus(uint64_t reqId, BREthereumTransaction* transactions, size_t transactionsCount, uint8_t**rlpBytes, size_t* rlpBytesSize) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem* items = (BRRlpItem*)malloc(sizeof(BRRlpItem)* (transactionsCount + 1));
    int idx = 0;
    
    items[idx++] = rlpEncodeItemUInt64(coder, 0x14,0);
    items[idx++] = rlpEncodeItemUInt64(coder, reqId,0);

    BRRlpItem* txtsItems = (BRRlpItem*)malloc(sizeof(BRRlpItem)* transactionsCount);
    for(int i = 0; i < transactionsCount; ++i){
        const BREthereumHash hash = transactionGetHash(transactions[i]);
        txtsItems[i] = rlpEncodeItemBytes(coder, (uint8_t*)hash.bytes, sizeof(hash.bytes));
    }
    items[idx++] = rlpEncodeListItems(coder, txtsItems, transactionsCount);

    BRRlpItem encoding = rlpEncodeListItems(coder, items, idx);
    rlpDataExtract(coder, encoding, rlpBytes, rlpBytesSize);
    rlpCoderRelease(coder);
    free(items);
    free(txtsItems);


}
BREthereumLESDecodeStatus ethereumLESDecodeTxStatus(uint8_t*rlpBytes, size_t rlpBytesSize, uint64_t* reqId, uint64_t* bv, BREthereumTransactionStatusReply** replies, size_t* repliesCount){

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpData data = {rlpBytesSize, rlpBytes};
    BRRlpItem item = rlpGetItem (coder, data);
    
    //Set default values for optional status values

    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    uint64_t messageId = rlpDecodeItemUInt64(coder, items[0],0);
    if(messageId != 0x15){
        return BRE_LES_INVALID_MSG_ID_ERROR;
    }
    *reqId = rlpDecodeItemUInt64(coder, items[1],0);
    *bv = rlpDecodeItemUInt64(coder, items[2],0);
    size_t statusesCount;
    const BRRlpItem *statuses = rlpDecodeList(coder, items[2], &statusesCount);
    
    BREthereumTransactionStatusReply*retReplies = (BREthereumTransactionStatusReply*)malloc(sizeof(BREthereumTransactionStatusReply) * statusesCount);
    for(int i = 0; i < statusesCount; ++i){
        size_t statusDataCount;
        const BRRlpItem* statusData = rlpDecodeList(coder, statuses[i], &statusDataCount);
        retReplies[i].status = rlpDecodeItemUInt64(coder, statusData[0],0);
        if(retReplies[i].status == TXSTATUS_INCLUDED){
            size_t includeDataCount;
            const BRRlpItem* includeData = rlpDecodeList(coder, statusData[1], &includeDataCount);
            BRRlpData data = rlpDecodeItemBytes(coder, includeData[0]);
            memcpy(retReplies[i].u.included_data.blockHash, data.bytes, data.bytesCount);
            rlpDataRelease(data);
            retReplies[i].u.included_data.blockNumber = rlpDecodeItemUInt64(coder, includeData[1],0);
            retReplies[i].u.included_data.txIndex = rlpDecodeItemUInt64(coder, includeData[2],0);
        }else if(retReplies[i].status == TXSTATUS_ERROR) {
            retReplies[i].u.error_message = rlpDecodeItemString(coder, statusData[1]);
        }
    }
    rlpCoderRelease(coder);

    *replies = retReplies;
    *repliesCount = statusesCount;

    return BRE_LES_SUCCESS;

}
void ethereumLESTxStatus( uint64_t reqId, uint64_t bv, BREthereumTransactionStatusReply* replies, size_t repliesCount, uint8_t**rlpBytes, size_t* rlpBytesSize){

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
