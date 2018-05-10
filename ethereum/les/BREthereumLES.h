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

#ifndef BR_Ethereum_LES_h
#define BR_Ethereum_LES_h

#include <inttypes.h>
#include "BREthereumBase.h"
#include "BREthereumTransaction.h"
#include "BREthereumBlock.h"
#include "BRInt.h"

#ifdef __cplusplus
extern "C" {
#endif

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
}BREthereumLESHeader;


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
}BREthereumLESStatusV1;

typedef struct {
    BREthereumLESStatusV1 v1Status;
    uint64_t announceType;
}BREthereumLESStatusV2;


//
// LES Reply Structures
//
 typedef struct {
    UInt256 parentHash;
    UInt256 ommersHash;
    uint8_t beneficiary[160];
    UInt256 stateRoot;
    UInt256 transactionsRoot;
    UInt256 receiptsRoot;
    UInt256 logsBloom;
    uint64_t diffculty;
    uint64_t number;
    uint64_t gasLimt;
    uint64_t gasUsed;
    uint64_t timeStamp;
    uint8_t extraData[32];
    UInt256 mixHash;
    uint8_t nonce[64];
}BREthereumBlockHeader;

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
    void* data;
}BREthereumTransactionStatusReply;


//
// LES Request Structures
//
typedef struct {
    BREthereumBoolean isBlockNumber;
    union {
        uint64_t blockNumber;
        UInt256 blockHash;
    }u;
}BREthereumBlockHeaderRequest;

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
/**
 * Encode a status message (LES V1)
 */
extern void ethereumLESEncodeV1Status(BREthereumLESStatusV1* status, uint8_t**rlpBytes, size_t* rlpBytesSize);

/**
 * Encode a status message (LES V2)
 */
extern void ethereumLESEncodeLESV2Status(BREthereumLESStatusV2* header, uint8_t**rlpBytes, size_t* rlpBytesSize);
/**
 * Decode a status message (LES V1) reply
 */
extern BREthereumLESDecodeStatus ethereumLESDecodeV1Status(uint8_t*rlpBytes, size_t rlpBytesSize, BREthereumLESStatusV1* status);

/**
 * Decode a status message (LES V2) reply
 */
extern BREthereumLESDecodeStatus ethereumLESDecodeLESV2Status(uint8_t*rlpBytes, size_t rlpBytesSize, BREthereumLESStatusV2* status);

/*********/

//
//  Header synchronisation
//

/**
 * Encode an Announce message
 */
extern void ethereumLESAnnounce(UInt256 headHash, uint64_t headNumber, uint64_t headTd, uint64_t reorgDepth, size_t flowControlMRRCount,
                               BREthereumAnnounceRequest* handshakeVals, size_t handshakeValsCount,
                               uint8_t**rlpBytes, size_t* rlpByesSize) ;

/**
 * Encode a GetBlockHeaders message
 */
extern void ethereumLESGetBlockHeaders(uint64_t reqId,
                                      BREthereumBlockHeaderRequest*configs, size_t configCount,
                                      uint64_t maxHeaders,
                                      uint64_t skip,
                                      uint64_t reverse,
                                      uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Decode the reply from a GetBlockHeaders message (i.e. BlockHeaders)
 */
extern void ethereumLESDecodeBlockHeaders(uint8_t*rlpBytes, BREthereumBlockHeader* blockHeader);

/**
 * Encode a BlockHeaders
 */
extern void ethereumLESBlockHeaders(uint64_t reqId, uint64_t bv, const BREthereumBlockHeader* blockHeader,  uint8_t**rlpBytes, size_t* rlpByesSize);


/*********/

//
// On-demand data retrieval
//
/**
 * Encode a GetBlockBodies  message
 */
extern void ethereumLESGetBlockBodies(UInt256* blockHashes, size_t blockHashesCount, uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Decode the reply from a GetBlockBodies message (i.e. BlockBodies)
 */
extern void ethereumLESDecodeBlockBodies(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv, BREthereumBlockBody** blockBodies, size_t* blockBodiesCount);

/**
 * Encode a BlockBodies message
 */
extern void ethereumLESBlockBodies(uint64_t reqId, uint64_t bv, const BREthereumBlockBody* blockBodies, size_t blockBoidesCount, uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Encode a GetReceipts message
 */
extern void ethereumLESGetReceipts(uint64_t reqId, UInt256* receipts, size_t receiptsCount, uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Decode the reply from a GetReceipts message (i.e. Receipts)
 */
extern void ethereumLESDecodeReceipts(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv, BREthereumReceipt**receipts, size_t* receiptsCount);

/**
 * Encode a Receipts message
 */
extern void ethereumLESReceipts(uint64_t reqId, uint64_t bv, BREthereumReceipt* receipts, size_t receiptsCount, uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Encode a GetProofs message
 */
extern void ethereumLESGetProofs(uint64_t reqId, BREthereumProofsRequest* proofs, size_t proofsCount,  uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Decode the reply from a GetProofs message (i.e. Proofs)
 */
extern void ethereumLESDecodeProofs(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv,  BREthereumProofNode** proofs, size_t* proofsCount);

/**
 * Encode a GetContractCodes message
 */
extern void ethereumLESGetContractCodes(uint64_t reqId,BREthereumContractCodesRequest* contractCodes, size_t contractCodesCount);

/**
 * Decode ther reply from a GetContractCodes message (i.e. ContractCodes)
 */
extern void ethereumLESDecodeContractCodes(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv,  uint8_t** contractCodes, size_t* contractCodesCount);

/**
 * Encode a ContractCodes message
 */
extern void ethereumLESContractCodes(uint64_t reqId, uint64_t bv, uint8_t* contractCodes, size_t contractCodesCount, uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Encode a GetHeaderProofs message
 */
extern void ethereumLESGetHeaderProofs(uint64_t reqId, BREthereumHeaderProofRequest* headerProofs, size_t headerProofsCount);

/**
 * Decode the reply from a GetHeaderProofs message (i.e. HeaderProofs)
 */
extern void ethereumLESDecodeHeaderProofs(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv, BREthereumHeaderProof** headerProofs, size_t* headerProofsCount);

/**
 * Encode a HeaderProofs message
 */
extern void ethereumLESHeaderProofs(uint64_t reqId, uint64_t bv, BREthereumHeaderProof* headerProofs, size_t headerProofsCount,uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Encode a GetProofsV2 message
 */
extern void ethereumLESGetProofsV2(uint64_t reqId, BREthereumProofsRequest* proofs, size_t proofsCount,  uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Decode ther reply from a GetProofsV2 message (i.e. ProofsV2)
 */
extern void ethereumLESDecodeProofsV2(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv,  BREthereumProofNode** proofs, size_t* proofsCount);

/**
 * Encode a ProofsV2 message
 */
extern void ethereumLESProofsV2(uint64_t reqId, uint64_t bv,  BREthereumProofNode* proofs, size_t proofsCount,uint8_t**rlpBytes, size_t* rlpByesSize);


/**
 * Encode a GetHelperTrieProofs messsage
 */
extern void ethereumLESGetHelperTrieProofs(uint64_t reqId, BREthereumGetHelperTrieProofsRequest* trieProofs, size_t trieProofsCount,  uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Decode the reply from a GetHelperTrieProofs message (i.e. HelperTrieProofs)
 */
extern void ethereumLESDecodeHelperTrieProofs(uint8_t*rlpBytes,  uint64_t totalAuxReqs, uint64_t* reqId, uint64_t* bv, BREthereumHelperTrieProofs** proofs,  size_t* trieProofsCount);

/**
 * Encode a HelperTrieProofs message
 */
 extern void ethereumLESHelperTrieProofs(uint64_t reqId, uint64_t bv,  BREthereumHelperTrieProofs* proofs,  size_t trieProofsCount, uint8_t**rlpBytes, size_t* rlpByesSize);


//
// Transaction relaying and status retrieval
//
/**
 * Encode a SendTxt message
 */
extern void ethereumLESSendTxt(uint64_t reqId, BREthereumTransaction* transactions, size_t transactionsCount, uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Encode a SendTxtV2 message
 */
extern void ethereumLESSendTxtV2(uint64_t reqId, BREthereumTransaction* transactions, size_t transactionsCount, uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Encode a GetTxStatus message
 */
extern void ethereumLESGetTxStatus(uint64_t reqId, BREthereumTransaction* transaction, size_t txCount, uint8_t**rlpBytes, size_t* rlpByesSize);

/**
 * Decode a GetTxStatus request message
 */
extern void ethereumLESDecodeTxStatus(uint8_t*rlpBytes, uint64_t* reqId, uint64_t* bv, BREthereumTransactionStatusReply** replies, size_t* repliesCount);

/**
 * Encode a TxStatus message
 */
extern void ethereumLESTxStatus( uint64_t reqId, uint64_t bv, BREthereumTransactionStatusReply* replies, size_t repliesCount, uint8_t**rlpBytes, size_t* rlpByesSize);



/*********/


#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_h */
