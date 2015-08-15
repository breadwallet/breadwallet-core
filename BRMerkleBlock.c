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
#include <limits.h>
#include <string.h>
#include <math.h>

#define VAR_INT16_HEADER 0xfd
#define VAR_INT32_HEADER 0xfe
#define VAR_INT64_HEADER 0xff

unsigned long long BRVarInt(const char *buf, size_t len, size_t *intLen)
{
    unsigned char h = (sizeof(char) <= len) ? *(const unsigned char *)buf : 0;
    
    switch (h) {
        case VAR_INT16_HEADER:
            if (intLen) *intLen = sizeof(h) + sizeof(short);
            return (sizeof(h) + sizeof(short) <= len) ? BRLE16(*(const unsigned short *)(buf + sizeof(h))) : 0;
            
        case VAR_INT32_HEADER:
            if (intLen) *intLen = sizeof(h) + sizeof(int);
            return (sizeof(h) + sizeof(int) <= len) ? BRLE32(*(const unsigned *)(buf + sizeof(h))) : 0;
            
        case VAR_INT64_HEADER:
            if (intLen) *intLen = sizeof(h) + sizeof(long long);
            return (sizeof(h) + sizeof(long long) <= len) ? BRLE64(*(const unsigned long long *)(buf + sizeof(h))) : 0;
            
        default:
            if (intLen) *intLen = sizeof(h);
            return h;
    }
}

size_t BRVarIntSize(unsigned long long i)
{
    if (i < VAR_INT16_HEADER) return sizeof(unsigned char);
    else if (i <= USHRT_MAX) return sizeof(unsigned char) + sizeof(unsigned short);
    else if (i <= UINT_MAX) return sizeof(unsigned char) + sizeof(unsigned);
    else return sizeof(unsigned char) + sizeof(unsigned long long);
}

size_t BRSetVarInt(unsigned long long i, char *buf, size_t len)
{
    if (i < VAR_INT16_HEADER) {
        if (sizeof(unsigned char) > len) return 0;
        *(unsigned char *)buf = (unsigned char)i;
        return sizeof(unsigned char);
    }
    else if (i <= USHRT_MAX) {
        if (sizeof(unsigned char) + sizeof(unsigned short) > len) return 0;
        *(unsigned char *)buf = VAR_INT16_HEADER;
        *(unsigned short *)(buf + sizeof(unsigned char)) = BRLE16((unsigned short)i);
        return sizeof(unsigned char) + sizeof(unsigned short);
    }
    else if (i <= UINT_MAX) {
        if (sizeof(unsigned char) + sizeof(unsigned) > len) return 0;
        *(unsigned char *)buf = VAR_INT32_HEADER;
        *(unsigned *)(buf + sizeof(unsigned char)) = BRLE32((unsigned)i);
        return sizeof(unsigned char) + sizeof(unsigned);
    }
    else {
        if (sizeof(unsigned char) + sizeof(unsigned long long) > len) return 0;
        *(unsigned char *)buf = VAR_INT64_HEADER;
        *(unsigned long long *)(buf + sizeof(unsigned char)) = BRLE64(i);
        return sizeof(unsigned char) + sizeof(unsigned long long);
    }
}

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

static size_t BRMerkleBlockTxHashesR(BRMerkleBlock *block, BRUInt256 *txHashes, size_t count, size_t *idx,
                                     size_t *hashIdx, size_t *flagIdx, int depth)
{
    if (*flagIdx/8 >= block->flagsLen || *hashIdx >= block->hashesLen) return 0;
    
    char flag = (block->flags[*flagIdx/8] & (1 << (*flagIdx % 8)));
    
    (*flagIdx)++;
    
    if (! flag || depth == (int)(ceil(log2(block->totalTransactions)))) {
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

// populates txHashes with the matched tx hashes in the block, returns number of hashes written, or total number if
// txHashes is NULL
size_t BRMerkleBlockTxHashes(BRMerkleBlock *block, BRUInt256 *txHashes, size_t count)
{
    size_t idx = 0, hashIdx = 0, flagIdx = 0;

    return BRMerkleBlockTxHashesR(block, txHashes, (txHashes) ? count : SIZE_MAX, &idx, &hashIdx, &flagIdx, 0);
}

// recursively walks the merkle tree to calculate the merkle root
static BRUInt256 BRMerkleBlockRootR(BRMerkleBlock *block, size_t *hashIdx, size_t *flagIdx, int depth)
{
    if (*flagIdx/8 >= block->flagsLen || *hashIdx >= block->hashesLen) return BRUINT256_ZERO;
    
    char flag = (block->flags[*flagIdx/8] & (1 << (*flagIdx % 8)));
    BRUInt256 hashes[2];
    
    (*flagIdx)++;
    if (! flag || depth == (int)(ceil(log2(block->totalTransactions)))) return block->hashes[(*hashIdx)++]; // leaf

    hashes[0] = BRMerkleBlockRootR(block, hashIdx, flagIdx, depth + 1); // left branch
    hashes[1] = BRMerkleBlockRootR(block, hashIdx, flagIdx, depth + 1); // right branch
    if (br_uint256_is_zero(hashes[1])) hashes[1] = hashes[0]; // if right branch is missing, duplicate left branch
    return BRSHA256_2(hashes, sizeof(hashes));
}

// true if merkle tree and timestamp are valid, and proof-of-work matches the stated difficulty target
// NOTE: This only checks if the block difficulty matches the difficulty target in the header. It does not check if the
// target is correct for the block's height in the chain. Use br_merkleblock_verify_difficulty for that.
int BRMerkleBlockIsValid(BRMerkleBlock *block, unsigned currentTime)
{
    static const unsigned maxsize = MAX_PROOF_OF_WORK >> 24, maxtarget = MAX_PROOF_OF_WORK & 0x00ffffffu;
    const unsigned size = block->target >> 24, target = block->target & 0x00ffffffu;
    size_t hashIdx = 0, flagIdx = 0;
    BRUInt256 merkleRoot = BRMerkleBlockRootR(block, &hashIdx, &flagIdx, 0), t = BRUINT256_ZERO;
    
    // check if merkle root is correct
    if (block->totalTransactions > 0 && ! br_uint256_eq(merkleRoot, block->merkleRoot)) return 0;
    
    // check if timestamp is too far in future
    if (block->timestamp > currentTime + MAX_TIME_DRIFT) return 0;
    
    // check if proof-of-work target is out of range
    if (target == 0 || target & 0x00800000u || size > maxsize || (size == maxsize && target > maxtarget)) return 0;
    
    if (size > 3) *(unsigned *)&t.u8[size - 3] = BRLE32(target);
    else t.u32[0] = BRLE32(target >> (3 - size)*8);
    
    for (int i = sizeof(t)/sizeof(unsigned) - 1; i >= 0; i--) { // check proof-of-work
        if (BRLE32(block->blockHash.u32[i]) < BRLE32(t.u32[i])) break;
        if (BRLE32(block->blockHash.u32[i]) > BRLE32(t.u32[i])) return 0;
    }
    
    return 1;
}

// returns number of bytes written to buf, or total size needed if buf is NULL
size_t BRMerkleBlockSerialize(BRMerkleBlock *block, char *buf, size_t len)
{
    size_t off = 0, l = 80;
    
    if (block->totalTransactions > 0) {
        l += sizeof(unsigned) + BRVarIntSize(block->hashesLen) + block->hashesLen*sizeof(BRUInt256) +
             BRVarIntSize(block->flagsLen) + block->flagsLen;
    }

    if (! buf) return l;
    if (len < l) return 0;
    *(unsigned *)(buf + off) = BRLE32(block->version);
    off += sizeof(unsigned);
    *(BRUInt256 *)(buf + off) = block->prevBlock;
    off += sizeof(BRUInt256);
    *(BRUInt256 *)(buf + off) = block->merkleRoot;
    off += sizeof(BRUInt256);
    *(unsigned *)(buf + off) = BRLE32(block->timestamp);
    off += sizeof(unsigned);
    *(unsigned *)(buf + off) = BRLE32(block->target);
    off += sizeof(unsigned);
    *(unsigned *)(buf + off) = BRLE32(block->nonce);
    off += sizeof(unsigned);

    if (block->totalTransactions > 0) {
        *(unsigned *)(buf + off) = BRLE32(block->totalTransactions);
        off += sizeof(unsigned);
        off += BRSetVarInt(block->hashesLen, buf + off, len - off);
        memcpy(buf + off, block->hashes, block->hashesLen*sizeof(BRUInt256));
        off += block->hashesLen*sizeof(BRUInt256);
        memcpy(buf + off, block->flags, block->flagsLen);
        off += block->flagsLen;
    }
    
    return l;
}

// buf can contain either a serialized merkleblock or header, returns true on success, keeps a reference to buf data
int BRMerkleBlockDeserialize(BRMerkleBlock *block, const char *buf, size_t len)
{
    if (len < 80) return 0;
    
    size_t off = 0, l = 0;
    char header[80];
    
    block->version = BRLE32(*(const unsigned *)(buf + off));
    off += sizeof(unsigned);
    block->prevBlock = *(const BRUInt256 *)(buf + off);
    off += sizeof(BRUInt256);
    block->merkleRoot = *(const BRUInt256 *)(buf + off);
    off += sizeof(BRUInt256);
    block->timestamp = BRLE32(*(const unsigned *)(buf + off));
    off += sizeof(unsigned);
    block->target = BRLE32(*(const unsigned *)(buf + off));
    off += sizeof(unsigned);
    block->nonce = BRLE32(*(const unsigned *)(buf + off));
    off += sizeof(unsigned);
    
    block->totalTransactions = (off + sizeof(unsigned) <= len) ? BRLE32(*(const unsigned *)(buf + off)) : 0;
    off += sizeof(unsigned);
    block->hashesLen = (size_t)BRVarInt(buf + off, len - off, &l);
    off += l;
    block->hashes = (off + block->hashesLen*sizeof(BRUInt256) <= len) ? (BRUInt256 *)(buf + off) : NULL;
    off += block->hashesLen*sizeof(BRUInt256);
    block->flagsLen = (size_t)BRVarInt(buf + off, len - off, &l);
    off += l;
    block->flags = (off + block->flagsLen <= len) ? buf + off : NULL;
    
    off = 0;
    *(unsigned *)(header + off) = BRLE32(block->version);
    off += sizeof(unsigned);
    *(BRUInt256 *)(header + off) = block->prevBlock;
    off += sizeof(BRUInt256);
    *(BRUInt256 *)(header + off) = block->merkleRoot;
    off += sizeof(BRUInt256);
    *(unsigned *)(header + off) = BRLE32(block->timestamp);
    off += sizeof(unsigned);
    *(unsigned *)(header + off) = BRLE32(block->target);
    off += sizeof(unsigned);
    *(unsigned *)(header + off) = BRLE32(block->nonce);
    block->blockHash = BRSHA256_2(header, sizeof(header));
    
    block->height = BR_BLOCK_UNKNOWN_HEIGHT;
    return 1;
}

// true if the given tx hash is known to be included in the block
int BRMerkleBlockContainsTxHash(BRMerkleBlock *block, BRUInt256 txHash)
{
    for (size_t i = 0; i < block->hashesLen; i++) {
        if (br_uint256_eq(block->hashes[i], txHash)) return 1;
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
int BRMerkleBlockVerifyDifficulty(BRMerkleBlock *block, BRMerkleBlock *previous, unsigned transitionTime)
{
    if (! br_uint256_eq(block->prevBlock, previous->blockHash) || block->height != previous->height + 1) return 0;
    if ((block->height % BR_BLOCK_DIFFICULTY_INTERVAL) == 0 && transitionTime == 0) return 0;
    
#if BITCOIN_TESTNET
    //TODO: implement testnet difficulty rule check
    return 1; // don't worry about difficulty on testnet for now
#endif
    
    if ((block->height % BR_BLOCK_DIFFICULTY_INTERVAL) != 0) return (block->target == previous->target) ? 1 : 0;
    
    // target is in "compact" format, explained in isValid:
    static const unsigned maxsize = MAX_PROOF_OF_WORK >> 24, maxtarget = MAX_PROOF_OF_WORK & 0x00ffffffu;
    int timespan = (int)((long long)previous->timestamp - (long long)transitionTime), size = previous->target >> 24;
    unsigned long long target = previous->target & 0x00ffffffu;
    
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
    
    return (block->target == ((unsigned)target | size << 24)) ? 1 : 0;
}

// returns a hash value suitable for including block in a hashtable
inline unsigned BRMerkleBlockHash(BRMerkleBlock *block)
{
    return block->blockHash.u32[0];
}

// true if block is equal to otherBlock
inline int BRMerkleBlockEqual(BRMerkleBlock *block, BRMerkleBlock *otherBlock)
{
    return br_uint256_eq(block->blockHash, otherBlock->blockHash);
}

