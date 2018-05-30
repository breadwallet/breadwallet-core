//
//  BREthereumNode.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 5/29/18.
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

#ifndef BR_Ethereum_LES_Coder_h
#define BR_Ethereum_LES_Coder_h

#include <stdio.h>
#include <inttypes.h>
#include "BREthereumBase.h"
#include "BREthereumTransaction.h"
#include "BREthereumTransactionReceipt.h"
#include "BREthereumTransactionStatus.h"
#include "BREthereumBlock.h"
#include "BRKey.h"
#include "BRArray.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    int foo;
}BREthereumReceipt;



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










#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Coder_h */
