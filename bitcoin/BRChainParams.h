//
//  BRChainParams.h
//
//  Created by Aaron Voisine on 1/10/18.
//  Copyright (c) 2018-2019 breadwallet LLC
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

#ifndef BRChainParams_h
#define BRChainParams_h

#include "BRMerkleBlock.h"
#include "BRSet.h"
#include "BRPeer.h"
#include "BRAddress.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#define BITCOIN_FORKID 0x00

typedef struct {
    uint32_t height;
    UInt256 hash;
    uint32_t timestamp;
    uint32_t target;
} BRCheckPoint;

typedef struct {
    const char * const *dnsSeeds; // NULL terminated array of dns seeds
    uint16_t standardPort;
    uint32_t magicNumber;
    uint64_t services;
    int (*verifyDifficulty)(const BRMerkleBlock *block, const BRSet *blockSet); // blockSet must have last 2016 blocks
    const BRCheckPoint *checkpoints;
    size_t checkpointsCount;
    BRAddressParams addrParams;
    uint8_t forkId;
} BRChainParams;

extern const BRChainParams *BRMainNetParams;
extern const BRChainParams *BRTestNetParams;

static inline const BRChainParams *BRChainParamsGetBitcoin (int mainnet) {
    return mainnet ? BRMainNetParams : BRTestNetParams;
}

static inline int BRChainParamsIsBitcoin (const BRChainParams *params) {
    return BRMainNetParams == params || BRTestNetParams == params;
}

extern const BRCheckPoint *BRChainParamsGetCheckpointBefore (const BRChainParams *params, uint32_t timestamp);

extern const BRCheckPoint *BRChainParamsGetCheckpointBeforeBlockNumber (const BRChainParams *params, uint32_t blockNumber);

#ifdef __cplusplus
}
#endif

#endif // BRChainParams_h
