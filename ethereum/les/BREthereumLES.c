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
#include "BRRlpCoder.h"
#include "BREthereumLES.h"
#include "BREthereumBase.h"

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

static void _encodeV1Status(BREthereumLESStatusV1* status, BRRlpCoder coder, BRRlpItem* statusItems, int* ioIdx) {

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
void ethereumLESEncodeV1Status(BREthereumLESStatusV1* status, uint8_t**rlpBytes, size_t* rlpBytesSize) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem statusItems[14];
    int ioIdx = 0;
    
    statusItems[ioIdx++] = rlpEncodeItemUInt64(coder, 0x00,0);
    _encodeV1Status(status,coder,statusItems,&ioIdx);
    BRRlpItem encoding = rlpEncodeListItems(coder, statusItems, ioIdx);
    rlpDataExtract(coder, encoding, rlpBytes, rlpBytesSize);
    rlpCoderRelease(coder);
}

void ethereumLESEncodeLESV2Status(BREthereumLESStatusV2* status, uint8_t**rlpBytes, size_t* rlpBytesSize) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem statusItems[15];
    int ioIdx = 0;
    
    statusItems[ioIdx++] = rlpEncodeItemUInt64(coder, 0x00,0);
    _encodeV1Status(&status->v1Status,coder,statusItems,&ioIdx);
    
    //announceType
    BRRlpItem keyPair [2];
    keyPair[0] = rlpEncodeItemString(coder, "announceType");
    keyPair[1] = rlpEncodeItemUInt64(coder, status->announceType,0);
    statusItems[ioIdx++] = rlpEncodeListItems(coder, keyPair, 2);

    BRRlpItem encoding = rlpEncodeListItems(coder, statusItems, ioIdx);
    rlpDataExtract(coder, encoding, rlpBytes, rlpBytesSize);
    rlpCoderRelease(coder);

}
static BREthereumLESDecodeStatus _decodeStatus(BRRlpCoder coder, const BRRlpItem *items, size_t itemsCount, BREthereumLESStatusV1* header){

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
BREthereumLESDecodeStatus ethereumLESDecodeV1Status(uint8_t*rlpBytes, size_t rlpBytesSize, BREthereumLESStatusV1* status) {

    
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
BREthereumLESDecodeStatus ethereumLESDecodeLESV2Status(uint8_t*rlpBytes, size_t rlpBytesSize, BREthereumLESStatusV2* status) {

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpData frameData = {rlpBytesSize, rlpBytes};
    BRRlpItem item = rlpGetItem (coder, frameData);
    
    //Set default values for optional status values
    status->v1Status.serveHeaders = ETHEREUM_BOOLEAN_FALSE;
    status->v1Status.flowControlBL = NULL;
    status->v1Status.flowControlMRC = NULL;
    status->v1Status.flowControlMRCCount = NULL;
    status->v1Status.flowControlMRR = NULL;
    status->v1Status.txRelay = ETHEREUM_BOOLEAN_FALSE;
    status->v1Status.serveChainSince = NULL;
    status->v1Status.serveStateSince = NULL;
    
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    
    BREthereumLESDecodeStatus retStatus = _decodeStatus(coder, items, itemsCount, &status->v1Status);
    
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

void ethereumLESGetBlockHeaders(uint64_t reqId,
                                      BREthereumBlockHeaderRequest*configs, size_t configCount,
                                      uint64_t maxHeaders,
                                      uint64_t skip,
                                      uint64_t reverse,
                                      uint8_t**rlpBytes, size_t* rlpByesSize) {
    
        //TODO: Encode the rlp header from EthereumBlock.h once implemented
}

void ethereumLESDecodeBlockHeaders(uint8_t*rlpBytes, BREthereumBlockHeader* blockHeader) {

    
       //TODO: Decode the rlp header from EthereumBlock.h once implemented

}

void ethereumLESBlockHeaders(uint64_t reqId, uint64_t bv, const BREthereumBlockHeader* blockHeader,  uint8_t**rlpBytes, size_t* rlpByesSize) {

       //TODO: Decode the rlp header from EthereumBlock.h once implemented

}


//
// On-demand data retrieval
//
void ethereumLESGetBlockBodies(UInt256* blockHashes, size_t blockHashesCount, uint8_t**rlpBytes, size_t* rlpByesSize) {


}

void ethereumLESDecodeBlockBodies(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv, BREthereumBlockBody** blockBodies, size_t* blockBodiesCount) {


}

void ethereumLESBlockBodies(uint64_t reqId, uint64_t bv, const BREthereumBlockBody* blockBodies, size_t blockBoidesCount, uint8_t**rlpBytes, size_t* rlpByesSize) {



}

void ethereumLESGetReceipts(uint64_t reqId, UInt256* receipts, size_t receiptsCount, uint8_t**rlpBytes, size_t* rlpByesSize) {


}

void ethereumLESDecodeReceipts(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv, BREthereumReceipt**receipts, size_t* receiptsCount) {


}

void ethereumLESReceipts(uint64_t reqId, uint64_t bv, BREthereumReceipt* receipts, size_t receiptsCount, uint8_t**rlpBytes, size_t* rlpByesSize) {


}

void ethereumLESGetProofs(uint64_t reqId, BREthereumProofsRequest* proofs, size_t proofsCount,  uint8_t**rlpBytes, size_t* rlpByesSize) {


}

void ethereumLESDecodeProofs(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv,  BREthereumProofNode** proofs, size_t* proofsCount) {


}

void ethereumLESGetContractCodes(uint64_t reqId,BREthereumContractCodesRequest* contractCodes, size_t contractCodesCount) {


}

void ethereumLESDecodeContractCodes(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv,  uint8_t** contractCodes, size_t* contractCodesCount) {


}

void ethereumLESContractCodes(uint64_t reqId, uint64_t bv, uint8_t* contractCodes, size_t contractCodesCount, uint8_t**rlpBytes, size_t* rlpByesSize) {


}

void ethereumLESGetHeaderProofs(uint64_t reqId, BREthereumHeaderProofRequest* headerProofs, size_t headerProofsCount) {


}

void ethereumLESDecodeHeaderProofs(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv, BREthereumHeaderProof** headerProofs, size_t* headerProofsCount){


}

void ethereumLESHeaderProofs(uint64_t reqId, uint64_t bv, BREthereumHeaderProof* headerProofs, size_t headerProofsCount,uint8_t**rlpBytes, size_t* rlpByesSize) {


}

void ethereumLESGetProofsV2(uint64_t reqId, BREthereumProofsRequest* proofs, size_t proofsCount,  uint8_t**rlpBytes, size_t* rlpByesSize) {


}

void ethereumLESDecodeProofsV2(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv,  BREthereumProofNode** proofs, size_t* proofsCount) {


}

void ethereumLESProofsV2(uint64_t reqId, uint64_t bv,  BREthereumProofNode* proofs, size_t proofsCount,uint8_t**rlpBytes, size_t* rlpByesSize) {



}

void ethereumLESGetHelperTrieProofs(uint64_t reqId, BREthereumGetHelperTrieProofsRequest* trieProofs, size_t trieProofsCount,  uint8_t**rlpBytes, size_t* rlpByesSize) {


}

void ethereumLESDecodeHelperTrieProofs(uint8_t*rlpBytes,  uint64_t totalAuxReqs, uint64_t* reqId, uint64_t* bv, BREthereumHelperTrieProofs** proofs,  size_t* trieProofsCount) {


}

void ethereumLESHelperTrieProofs(uint64_t reqId, uint64_t bv,  BREthereumHelperTrieProofs* proofs,  size_t trieProofsCount, uint8_t**rlpBytes, size_t* rlpByesSize) {


}


//
// Transaction relaying and status retrieval
//

void ethereumLESSendTxt(uint64_t reqId, BREthereumTransaction* transactions, size_t transactionsCount, uint8_t**rlpBytes, size_t* rlpByesSize) {


}
void ethereumLESSendTxtV2(uint64_t reqId, BREthereumTransaction* transactions, size_t transactionsCount, uint8_t**rlpBytes, size_t* rlpByesSize) {


}
void ethereumLESGetTxStatus(uint64_t reqId, BREthereumTransaction* transaction, size_t txCount, uint8_t**rlpBytes, size_t* rlpByesSize) {


}
void ethereumLESDecodeTxStatus(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv, BREthereumTransactionStatusReply** replies, size_t* repliesCount){


}

void ethereumLESTxStatus( uint64_t reqId, uint64_t bv, BREthereumTransactionStatusReply* replies, size_t repliesCount, uint8_t**rlpBytes, size_t* rlpByesSize){



}
