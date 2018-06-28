//
//  BBREthereumBlock.h
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/23/2018.
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

#ifndef BR_Ethereum_Block_H
#define BR_Ethereum_Block_H

#include <limits.h>
#include "../base/BREthereumBase.h"
#include "BREthereumTransaction.h"
#include "BREthereumBloomFilter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumBlockHeaderRecord *BREthereumBlockHeader;
typedef struct BREthereumBlockRecord *BREthereumBlock;

//
// Block Header
//
extern void
blockHeaderRelease (BREthereumBlockHeader header);
    
extern BREthereumBoolean
blockHeaderIsValid (BREthereumBlockHeader header);

extern BREthereumBlockHeader
blockHeaderDecodeRLP (BRRlpData data);

extern BRRlpData
blockHeaderEncodeRLP (BREthereumBlockHeader header,
                      BREthereumBoolean withNonce);

extern BREthereumHash
blockHeaderGetHash (BREthereumBlockHeader header);

extern uint64_t
blockHeaderGetNumber (BREthereumBlockHeader header);

extern uint64_t
blockHeaderGetDifficulty (BREthereumBlockHeader header);

extern uint64_t
blockHeaderGetGasUsed (BREthereumBlockHeader header);

extern uint64_t
blockHeaderGetTimestamp (BREthereumBlockHeader header);

extern BREthereumHash
blockHeaderGetParentHash (BREthereumBlockHeader header);

// ...

extern uint64_t
blockHeaderGetNonce (BREthereumBlockHeader header);

extern BREthereumBoolean
blockHeaderMatch (BREthereumBlockHeader header,
                  BREthereumBloomFilter filter);

extern BREthereumBoolean
blockHeaderMatchAddress (BREthereumBlockHeader header,
                         BREthereumAddress address);

// Support BRSet
extern size_t
blockHeaderHashValue (const void *h);

// Support BRSet
extern int
blockHeaderHashEqual (const void *h1, const void *h2);    

extern void
blockHeaderReleaseForSet (void *ignore, void *item);

// Support sorting
extern BREthereumComparison
blockHeaderCompare (BREthereumBlockHeader h1,
                    BREthereumBlockHeader h2);
    
//
// Block
//
extern BREthereumBlock
createBlockMinimal(BREthereumHash hash,
            uint64_t number,
            uint64_t timestamp);

extern BREthereumBlock
createBlock (BREthereumBlockHeader header,
             BREthereumBlockHeader ommers[], size_t ommersCount,
             BREthereumTransaction transactions[], size_t transactionCount);

extern BREthereumBlockHeader
blockHeaderCopy (BREthereumBlockHeader source);

extern void
blockRelease (BREthereumBlock block);

extern BREthereumBoolean
blockIsValid (BREthereumBlock block,
              BREthereumBoolean skipHeaderValidation);

extern BREthereumBlockHeader
blockGetHeader (BREthereumBlock block);

extern unsigned long
blockGetTransactionsCount (BREthereumBlock block);

extern BREthereumTransaction
blockGetTransaction (BREthereumBlock block, size_t index);

extern unsigned long
blockGetOmmersCount (BREthereumBlock block);

extern BREthereumBlockHeader
blockGetOmmer (BREthereumBlock block, unsigned int index);

extern BREthereumHash
blockGetHash (BREthereumBlock block);

extern uint64_t
blockGetNumber (BREthereumBlock block);

extern uint64_t
blockGetConfirmations (BREthereumBlock block);

extern uint64_t
blockGetTimestamp (BREthereumBlock block);

extern BRRlpData
blockEncodeRLP (BREthereumBlock block,
                BREthereumNetwork network);

extern BREthereumBlock
blockDecodeRLP (BRRlpData data,
                BREthereumNetwork network);

//
// Support LES decoding of BlockBodies
//

/**
 * Return BRArrayOf(BREthereumBlockHeader) w/ array owned by caller.
 */
extern BREthereumBlockHeader *
blockOmmersRlpDecodeItem (BRRlpItem item,
                          BREthereumNetwork network,
                          BRRlpCoder coder);

/**
 * Return BRArrayOf(BREthereumTransaction) w/ array owned by caller
 */
extern BREthereumTransaction *
blockTransactionsRlpDecodeItem (BRRlpItem item,
                                BREthereumNetwork network,
                                BRRlpCoder coder);
//
// Genesis Blocks
//

/**
 * Return a newly-allocaed block header duplicating the genesis block header for `netwrok`.
 */
extern const BREthereumBlockHeader
networkGetGenesisBlockHeader (BREthereumNetwork network);

//
// Checkpoint
//
typedef struct {
    uint64_t number;
    BREthereumHash hash;
    uint64_t timestamp;
} BREthereumBlockCheckpoint;

#define BLOCK_CHECKPOINT_LAST_NUMBER      (UINT64_MAX)
#define BLOCK_CHECKPOINT_LAST_TIMESTAMP   (UINT64_MAX)

extern const BREthereumBlockCheckpoint *
blockCheckpointLookupLatest (BREthereumNetwork network);

extern const BREthereumBlockCheckpoint *
blockCheckpointLookupByNumber (BREthereumNetwork network,
                               uint64_t number);

extern const BREthereumBlockCheckpoint *
blockCheckpointLookupByTimestamp (BREthereumNetwork network,
                                  uint64_t timestamp);

extern BREthereumBlockHeader
blockCheckpointCreatePartialBlockHeader (const BREthereumBlockCheckpoint *checkpoint);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Block_H */
