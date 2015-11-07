//
//  BRMerkleBlock.c
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

#include "BRMerkleBlock.h"
#include "BRHash.h"
#include "BRAddress.h"
#include <limits.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define MAX_TIME_DRIFT    (2*60*60)     // the furthest in the future a block is allowed to be timestamped
#define MAX_PROOF_OF_WORK 0x1d00ffffu   // highest value for difficulty target (higher values are less difficult)
#define TARGET_TIMESPAN   (14*24*60*60) // the targeted timespan between difficulty target adjustments

// from https://en.bitcoin.it/wiki/Protocol_specification#Merkle_Trees
// Merkle trees are binary trees of hashes. Merkle trees in bitcoin use a double SHA-256, the SHA-256 hash of the
// SHA-256 hash of something. If, when forming a row in the tree (other than the root of the tree), it would have an odd
// number of elements, the final double-hash is duplicated to ensure that the row has an even number of hashes. First
// form the bottom row of the tree with the ordered double-SHA-256 hashes of the byte streams of the transactions in the
// block. Then the row above it consists of half that number of hashes. Each entry is the double-SHA-256 of the 64-byte
// concatenation of the corresponding two hashes below it in the tree. This procedure repeats recursively until we reach
// a row consisting of just a single double-hash. This is the merkle root of the tree.
//
// from https://github.com/bitcoin/bips/blob/master/bip-0037.mediawiki#Partial_Merkle_branch_format
// The encoding works as follows: we traverse the tree in depth-first order, storing a bit for each traversed node,
// signifying whether the node is the parent of at least one matched leaf txid (or a matched txid itself). In case we
// are at the leaf level, or this bit is 0, its merkle node hash is stored, and its children are not explored further.
// Otherwise, no hash is stored, but we recurse into both (or the only) child branch. During decoding, the same
// depth-first traversal is performed, consuming bits and hashes as they were written during encoding.
//
// example tree with three transactions, where only tx2 is matched by the bloom filter:
//
//     merkleRoot
//      /     \
//    m1       m2
//   /  \     /  \
// tx1  tx2 tx3  tx3
//
// flag bits (little endian): 00001011 [merkleRoot = 1, m1 = 1, tx1 = 0, tx2 = 1, m2 = 0, byte padding = 000]
// hashes: [tx1, tx2, m2]

// returns a newly allocated BRMerkleBlock struct that must be freed by calling BRMerkleBlockFree()
BRMerkleBlock *BRMerkleBlockNew()
{
    BRMerkleBlock *block = calloc(1, sizeof(BRMerkleBlock));
    
    return block;
}

// buf can contain either a serialized merkleblock or header, result must be freed by calling BRMerkleBlockFree()
BRMerkleBlock *BRMerkleBlockParse(const uint8_t *buf, size_t len)
{
    if (! buf || len < 80) return NULL;
    
    BRMerkleBlock *block = malloc(sizeof(BRMerkleBlock));
    size_t off = 0, l = 0;
    uint8_t header[80];
    
    block->version = le32(*(const uint32_t *)(buf + off));
    off += sizeof(uint32_t);
    block->prevBlock = *(const UInt256 *)(buf + off);
    off += sizeof(UInt256);
    block->merkleRoot = *(const UInt256 *)(buf + off);
    off += sizeof(UInt256);
    block->timestamp = le32(*(const uint32_t *)(buf + off));
    off += sizeof(uint32_t);
    block->target = le32(*(const uint32_t *)(buf + off));
    off += sizeof(uint32_t);
    block->nonce = le32(*(const uint32_t *)(buf + off));
    off += sizeof(uint32_t);
    
    block->totalTx = (off + sizeof(uint32_t) <= len) ? le32(*(const uint32_t *)(buf + off)) : 0;
    off += sizeof(uint32_t);
    block->hashesLen = (size_t)BRVarInt(buf + off, len - off, &l);
    off += l;
    l = block->hashesLen*sizeof(UInt256);
    block->hashes = (off + l <= len) ? malloc(l) : NULL;
    if (block->hashes) memcpy(block->hashes, buf + off, l);
    off += l;
    block->flagsLen = (size_t)BRVarInt(buf + off, len - off, &l);
    off += l;
    l = block->flagsLen;
    block->flags = (off + l <= len) ? malloc(l) : NULL;
    if (block->flags) memcpy(block->flags, buf + off, l);
    
    off = 0;
    *(uint32_t *)(header + off) = le32(block->version);
    off += sizeof(uint32_t);
    *(UInt256 *)(header + off) = block->prevBlock;
    off += sizeof(UInt256);
    *(UInt256 *)(header + off) = block->merkleRoot;
    off += sizeof(UInt256);
    *(uint32_t *)(header + off) = le32(block->timestamp);
    off += sizeof(uint32_t);
    *(uint32_t *)(header + off) = le32(block->target);
    off += sizeof(uint32_t);
    *(uint32_t *)(header + off) = le32(block->nonce);
    BRSHA256_2(&block->blockHash, header, sizeof(header));
    
    block->height = BLOCK_UNKNOWN_HEIGHT;
    return block;
}

// returns number of bytes written to buf, or total len needed if buf is NULL
size_t BRMerkleBlockSerialize(BRMerkleBlock *block, uint8_t *buf, size_t len)
{
    size_t off = 0, l = 80;
    
    if (block->totalTx > 0) {
        l += sizeof(uint32_t) + BRVarIntSize(block->hashesLen) + block->hashesLen*sizeof(UInt256) +
             BRVarIntSize(block->flagsLen) + block->flagsLen;
    }
    
    if (! buf) return l;
    if (len < l) return 0;
    *(uint32_t *)(buf + off) = le32(block->version);
    off += sizeof(uint32_t);
    *(UInt256 *)(buf + off) = block->prevBlock;
    off += sizeof(UInt256);
    *(UInt256 *)(buf + off) = block->merkleRoot;
    off += sizeof(UInt256);
    *(uint32_t *)(buf + off) = le32(block->timestamp);
    off += sizeof(uint32_t);
    *(uint32_t *)(buf + off) = le32(block->target);
    off += sizeof(uint32_t);
    *(uint32_t *)(buf + off) = le32(block->nonce);
    off += sizeof(uint32_t);
    
    if (block->totalTx > 0) {
        *(uint32_t *)(buf + off) = le32(block->totalTx);
        off += sizeof(uint32_t);
        off += BRVarIntSet(buf + off, len - off, block->hashesLen);
        if (block->hashes) memcpy(buf + off, block->hashes, block->hashesLen*sizeof(UInt256));
        off += block->hashesLen*sizeof(UInt256);
        if (block->flags) memcpy(buf + off, block->flags, block->flagsLen);
        off += block->flagsLen;
    }
    
    return l;
}

static size_t BRMerkleBlockTxHashesR(BRMerkleBlock *block, UInt256 *txHashes, size_t count, size_t *idx,
                                     size_t *hashIdx, size_t *flagIdx, int depth)
{
    if (*flagIdx/8 >= block->flagsLen || *hashIdx >= block->hashesLen) return 0;
    
    uint8_t flag = (block->flags[*flagIdx/8] & (1 << (*flagIdx % 8)));
    
    (*flagIdx)++;
    
    if (! flag || depth == (int)(ceil(log2(block->totalTx)))) {
        if (flag && *idx < count) {
            if (txHashes) txHashes[*idx] = block->hashes[*hashIdx]; // leaf
            (*idx)++;
        }
        
        (*hashIdx)++;
        return *idx;
    }
    
    BRMerkleBlockTxHashesR(block, txHashes, count, idx, hashIdx, flagIdx, depth + 1); // left branch
    BRMerkleBlockTxHashesR(block, txHashes, count, idx, hashIdx, flagIdx, depth + 1); // right branch
    return *idx;
}

// populates txHashes with the matched tx hashes in the block, returns number of hashes written, or total number of
// matched hashes in the block if txHashes is NULL
size_t BRMerkleBlockTxHashes(BRMerkleBlock *block, UInt256 *txHashes, size_t count)
{
    size_t idx = 0, hashIdx = 0, flagIdx = 0;

    return BRMerkleBlockTxHashesR(block, txHashes, (txHashes) ? count : SIZE_MAX, &idx, &hashIdx, &flagIdx, 0);
}

// recursively walks the merkle tree to calculate the merkle root
static UInt256 BRMerkleBlockRootR(BRMerkleBlock *block, size_t *hashIdx, size_t *flagIdx, int depth)
{
    if (*flagIdx/8 >= block->flagsLen || *hashIdx >= block->hashesLen) return UINT256_ZERO;
    
    uint8_t flag = (block->flags[*flagIdx/8] & (1 << (*flagIdx % 8)));
    UInt256 hashes[2], md;
    
    (*flagIdx)++;
    if (! flag || depth == (int)(ceil(log2(block->totalTx)))) return block->hashes[(*hashIdx)++]; // leaf

    hashes[0] = BRMerkleBlockRootR(block, hashIdx, flagIdx, depth + 1); // left branch
    hashes[1] = BRMerkleBlockRootR(block, hashIdx, flagIdx, depth + 1); // right branch
    if (UInt256IsZero(hashes[1])) hashes[1] = hashes[0]; // if right branch is missing, duplicate left branch
    BRSHA256_2(&md, hashes, sizeof(hashes));
    return md;
}

// true if merkle tree and timestamp are valid, and proof-of-work matches the stated difficulty target
// NOTE: This only checks if the block difficulty matches the difficulty target in the header. It does not check if the
// target is correct for the block's height in the chain. Use BRMerkleBlockVerifyDifficulty() for that.
int BRMerkleBlockIsValid(BRMerkleBlock *block, unsigned currentTime)
{
    // target is in "compact" format, where the most significant byte is the size of resulting value in bytes, the next
    // bit is the sign, and the remaining 23bits is the value after having been right shifted by (size - 3)*8 bits
    static const uint32_t maxsize = MAX_PROOF_OF_WORK >> 24, maxtarget = MAX_PROOF_OF_WORK & 0x00ffffffu;
    const uint32_t size = block->target >> 24, target = block->target & 0x00ffffffu;
    size_t hashIdx = 0, flagIdx = 0;
    UInt256 merkleRoot = BRMerkleBlockRootR(block, &hashIdx, &flagIdx, 0), t = UINT256_ZERO;
    
    // check if merkle root is correct
    if (block->totalTx > 0 && ! UInt256Eq(merkleRoot, block->merkleRoot)) return 0;
    
    // check if timestamp is too far in future
    if (block->timestamp > currentTime + MAX_TIME_DRIFT) return 0;
    
    // check if proof-of-work target is out of range
    if (target == 0 || target & 0x00800000u || size > maxsize || (size == maxsize && target > maxtarget)) return 0;
    
    if (size > 3) *(uint32_t *)&t.u8[size - 3] = le32(target);
    else t.u32[0] = le32(target >> (3 - size)*8);
    
    for (int i = sizeof(t)/sizeof(uint32_t) - 1; i >= 0; i--) { // check proof-of-work
        if (le32(block->blockHash.u32[i]) < le32(t.u32[i])) break;
        if (le32(block->blockHash.u32[i]) > le32(t.u32[i])) return 0;
    }
    
    return 1;
}

// true if the given tx hash is known to be included in the block
int BRMerkleBlockContainsTxHash(BRMerkleBlock *block, UInt256 txHash)
{
    for (size_t i = 0; i < block->hashesLen; i++) {
        if (UInt256Eq(block->hashes[i], txHash)) return 1;
    }
    
    return 0;
}

// Verifies the block difficulty target is correct for the block's position in the chain. Transition time may be 0 if
// height is not a multiple of BLOCK_DIFFICULTY_INTERVAL.
//
// The difficulty target algorithm works as follows:
// The target must be the same as in the previous block unless the block's height is a multiple of 2016. Every 2016
// blocks there is a difficulty transition where a new difficulty is calculated. The new target is the previous target
// multiplied by the time between the last transition block's timestamp and this one (in seconds), divided by the
// targeted time between transitions (14*24*60*60 seconds). If the new difficulty is more than 4x or less than 1/4 of
// the previous difficulty, the change is limited to either 4x or 1/4. There is also a minimum difficulty value
// intuitively named MAX_PROOF_OF_WORK... since larger values are less difficult.
int BRMerkleBlockVerifyDifficulty(BRMerkleBlock *block, const BRMerkleBlock *previous, uint32_t transitionTime)
{
    if (! UInt256Eq(block->prevBlock, previous->blockHash) || block->height != previous->height + 1) return 0;
    if ((block->height % BLOCK_DIFFICULTY_INTERVAL) == 0 && transitionTime == 0) return 0;
    
#if BITCOIN_TESTNET
    //TODO: implement testnet difficulty rule check
    return 1; // don't worry about difficulty on testnet for now
#endif
    
    if ((block->height % BLOCK_DIFFICULTY_INTERVAL) != 0) return (block->target == previous->target) ? 1 : 0;
    
    // target is in "compact" format, where the most significant byte is the size of resulting value in bytes, the next
    // bit is the sign, and the remaining 23bits is the value after having been right shifted by (size - 3)*8 bits
    static const uint32_t maxsize = MAX_PROOF_OF_WORK >> 24, maxtarget = MAX_PROOF_OF_WORK & 0x00ffffffu;
    int timespan = (int)((int64_t)previous->timestamp - (int64_t)transitionTime), size = previous->target >> 24;
    uint64_t target = previous->target & 0x00ffffffu;
    
    // limit difficulty transition to -75% or +400%
    if (timespan < TARGET_TIMESPAN/4) timespan = TARGET_TIMESPAN/4;
    if (timespan > TARGET_TIMESPAN*4) timespan = TARGET_TIMESPAN*4;
    
    // TARGET_TIMESPAN happens to be a multiple of 256, and since timespan is at least TARGET_TIMESPAN/4, we don't lose
    // precision when target is multiplied by timespan and then divided by TARGET_TIMESPAN/256
    target *= timespan;
    target /= TARGET_TIMESPAN >> 8;
    size--; // decrement size since we only divided by TARGET_TIMESPAN/256
    
    while (size < 1 || target > 0x007fffffULL) target >>= 8, size++; // normalize target for "compact" format
    
    // limit to MAX_PROOF_OF_WORK
    if (size > maxsize || (size == maxsize && target > maxtarget)) target = maxtarget, size = maxsize;
    
    return (block->target == ((uint32_t)target | size << 24));
}

// frees memory allocated by BRMerkleBlockParse
void BRMerkleBlockFree(BRMerkleBlock *block)
{
    if (block->hashes) free(block->hashes);
    if (block->flags) free(block->flags);
    free(block);
}
