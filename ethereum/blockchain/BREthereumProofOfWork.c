//
//  BREthereumProofOfWork.c
//  Core
//
//  Created by Ed Gamble on 12/14/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <stdarg.h>
#include "support/BRArray.h"
#include "support/BRSet.h"
#include "ethereum/rlp/BRRlp.h"
#include "BREthereumBlock.h"

#define POW_WORD_BYTES            (4)
#define POW_DATA_SET_INIT         (1 << 30)
#define POW_DATA_SET_GROWTH       (1 << 23)
#define POW_CACHE_INIT            (1 << 24)
#define POW_CACHE_GROWTH          (1 << 17)
#define POW_EPOCH                 (30000)
#define POW_MIX_BYTES             (128)
#define POW_HASH_BYTES            (64)
#define POW_PARENTS               (256)
#define POW_CACHE_ROUNDS          (3)
#define POW_ACCESSES              (64)

//
// Proof Of Work
//
struct BREthereumProofOfWorkStruct {
    int foo;
};

extern BREthereumProofOfWork
proofOfWorkCreate (void) {
    BREthereumProofOfWork pow = calloc (1, sizeof (struct BREthereumProofOfWorkStruct));

    pow->foo = 0;

    return pow;
}

extern void
proofOfWorkRelease (BREthereumProofOfWork pow) {
    free (pow);
}

#if defined (INCLUDE_UNUSED_FUNCTIONS)
static uint64_t
powEpoch (BREthereumBlockHeader header) {
    return blockHeaderGetNonce(header) / POW_EPOCH;
}

static uint64_t
powPrime (uint64_t x, uint64_t y) {
    return (0 == x % y
            ? x
            : powPrime (x - 2 * y, y));
}

static uint64_t
powDatasetSize (BREthereumBlockHeader header) {
    return powPrime (POW_DATA_SET_INIT + POW_DATA_SET_GROWTH * powEpoch(header) - POW_MIX_BYTES,
                     POW_MIX_BYTES);
}

static uint64_t
powCacheSize (BREthereumBlockHeader header) {
    return powPrime (POW_CACHE_INIT + POW_CACHE_GROWTH * powEpoch(header) - POW_HASH_BYTES,
                     POW_HASH_BYTES);
}

static BREthereumHash
powSeedHashInternal (uint64_t number) {
    BREthereumHash bytes;

    if (0 == number)
        memset (bytes.bytes, 0, 32);
    else
        bytes = powSeedHashInternal(number - POW_EPOCH);

    return hashCreateFromData ((BRRlpData) { 32, bytes.bytes });
}

static BREthereumHash
powSeedHash (BREthereumBlockHeader header) {
    return powSeedHashInternal(blockHeaderGetNumber(header));
}

static BREthereumData
powLittleRMH (BREthereumData x, uint64_t i, uint64_t n) {
    return x;
}

static BREthereumData
powBigRMH (BREthereumData x, uint64_t n) {
    for (uint64_t i = 0; i < n; i++)
        powLittleRMH(x, i, n);
    return x;
}

static BREthereumData
powCacheRounds (BREthereumData x, uint64_t y, uint64_t n) {
    return (0 == y
            ? x
            : (1 == y
               ? powBigRMH (x, n)
               : powCacheRounds (powBigRMH (x, n), y - 1, n)));
}

static void
powBigFNV (BREthereumData x,
           BREthereumData y) {
}

static BREthereumData
powMix (BREthereumData m,
        BREthereumData c,
        uint64_t i,
        uint64_t p) {
    return (0 == p
            ? m
            : m);
}

static BREthereumData
powParents (BREthereumData c,
            uint64_t i,
            uint64_t p,
            BREthereumData m) {
    return (p < POW_PARENTS - 1
            ? powParents(c, i, p + 1, powMix (m, c, i, p + 1))
            : powMix (m, c, i, p + 1));
}

static BREthereumData
powDatasetItem (BREthereumData c,
                uint64_t i) {
    return powParents(c, i, -i, (BREthereumData) { 0, NULL });
}
// On and On and On....
#endif

extern void
proofOfWorkGenerate(BREthereumProofOfWork pow,
                       BREthereumBlockHeader header) {
    //
}

extern void
proofOfWorkCompute (BREthereumProofOfWork pow,
                       BREthereumBlockHeader header,
                       UInt256 *n,
                       BREthereumHash *m) {
    assert (NULL != n && NULL != m);

    // On and on and on

    // TODO: Actually compute something.

    // For now, return values that allow subsequent use to succeed
    *n = UINT256_ZERO;
    *m = blockHeaderGetMixHash (header);
}
