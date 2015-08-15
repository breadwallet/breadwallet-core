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

#include "BRHash.h"
#include <stddef.h>

#define BR_BLOCK_DIFFICULTY_INTERVAL 2016    // number of blocks between difficulty target adjustments
#define BR_BLOCK_UNKNOWN_HEIGHT      INT_MAX

typedef struct _BRMerkleBlock {
    BRUInt256 blockHash;
    unsigned version;
    BRUInt256 prevBlock;
    BRUInt256 merkleRoot;
    unsigned timestamp; // time interval since unix epoch
    unsigned target;
    unsigned nonce;
    unsigned totalTransactions;
    const BRUInt256 *hashes;
    size_t hashesLen;
    const char *flags;
    size_t flagsLen;
    unsigned height;
} BRMerkleBlock;

// populates txHashes with the matched tx hashes in the block, returns number of tx hashes written, or total number if
// txHashes is NULL
size_t BRMerkleBlockTxHashes(BRMerkleBlock *block, BRUInt256 *txHashes, size_t count);

// true if merkle tree and timestamp are valid, and proof-of-work matches the stated difficulty target
// NOTE: This only checks if the block difficulty matches the difficulty target in the header. It does not check if the
// target is correct for the block's height in the chain. Use br_merkleblock_verify_difficulty for that.
int BRMerkleBlockIsValid(BRMerkleBlock *block, unsigned currentTime);

// returns number of bytes written to buf, or total size needed if buf is NULL
size_t BRMerkleBlockSerialize(BRMerkleBlock *block, char *buf, size_t len);

// buf can contain either a serialized merkleblock or header, returns true on success, keeps a reference to buf data
int BRMerkleBlockDeserialize(BRMerkleBlock *block, const char *buf, size_t len);

// true if the given tx hash is known to be included in the block
int BRMerkleBlockContainsTxHash(BRMerkleBlock *block, BRUInt256 txHash);

// Verifies the block difficulty target is correct for the block's position in the chain. Transition time may be 0 if
// height is not a multiple of BLOCK_DIFFICULTY_INTERVAL.
int BRMerkleBlockVerifyDifficulty(BRMerkleBlock *block, BRMerkleBlock *previous, unsigned transitionTime);

// returns a hash value suitable for including block in a hashtable
unsigned BRMerkleBlockHash(BRMerkleBlock *block);

// true if block is equal to otherBlock
int BRMerkleBlockEqual(BRMerkleBlock *block, BRMerkleBlock *otherBlock);

#endif // BRMerkleBlock_h
