//
//  BRMerkleBlock.h
//
//  Created by Aaron Voisine on 8/6/15.
//  Copyright (c) 2015 breadwallet LLC
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

#ifndef BRMerkleBlock_h
#define BRMerkleBlock_h

#include "BRTypes.h"
#include <stddef.h>

#define BLOCK_DIFFICULTY_INTERVAL 2016    // number of blocks between difficulty target adjustments
#define BLOCK_UNKNOWN_HEIGHT      INT32_MAX

typedef struct {
    UInt256 blockHash;
    uint32_t version;
    UInt256 prevBlock;
    UInt256 merkleRoot;
    uint32_t timestamp; // time interval since unix epoch
    uint32_t target;
    uint32_t nonce;
    uint32_t totalTx;
    UInt256 *hashes;
    size_t hashesLen;
    uint8_t *flags;
    size_t flagsLen;
    uint32_t height;
} BRMerkleBlock;

// returns a newly allocated BRMerkleBlock struct that must be freed by calling BRMerkleBlockFree()
BRMerkleBlock *BRMerkleBlockNew();

// buf must contain either a serialized merkleblock or header, result must be freed by calling BRMerkleBlockFree()
BRMerkleBlock *BRMerkleBlockParse(const uint8_t *buf, size_t len);

// returns number of bytes written to buf, or total len needed if buf is NULL
size_t BRMerkleBlockSerialize(BRMerkleBlock *block, uint8_t *buf, size_t len);

// populates txHashes with the matched tx hashes in the block, returns number of tx hashes written, or total number of
// matched hashes in the block if txHashes is NULL
size_t BRMerkleBlockTxHashes(BRMerkleBlock *block, UInt256 *txHashes, size_t count);

// true if merkle tree and timestamp are valid, and proof-of-work matches the stated difficulty target
// NOTE: This only checks if the block difficulty matches the difficulty target in the header. It does not check if the
// target is correct for the block's height in the chain. Use BRMerkleBlockVerifyDifficulty() for that.
int BRMerkleBlockIsValid(BRMerkleBlock *block, uint32_t currentTime);

// true if the given tx hash is known to be included in the block
int BRMerkleBlockContainsTxHash(BRMerkleBlock *block, UInt256 txHash);

// Verifies the block difficulty target is correct for the block's position in the chain. Transition time may be 0 if
// height is not a multiple of BLOCK_DIFFICULTY_INTERVAL.
int BRMerkleBlockVerifyDifficulty(BRMerkleBlock *block, const BRMerkleBlock *previous, uint32_t transitionTime);

// returns a hash value for block suitable for use in a hashtable
inline static size_t BRMerkleBlockHash(const void *block)
{
    return *(const size_t *)&((const BRMerkleBlock *)block)->blockHash;
}

// true if block and otherBlock have equal blockHash values
inline static int BRMerkleBlockEq(const void *block, const void *otherBlock)
{
    return UInt256Eq(((const BRMerkleBlock *)block)->blockHash, ((const BRMerkleBlock *)otherBlock)->blockHash);
}

// frees memory allocated for block
void BRMerkleBlockFree(BRMerkleBlock *block);

#endif // BRMerkleBlock_h
