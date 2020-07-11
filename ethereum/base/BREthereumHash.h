//
//  BREthereumHash.h
//  BRCore
//
//  Created by Ed Gamble on 5/17/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Hash_H
#define BR_Ethereum_Hash_H

#include <stdlib.h>
#include <memory.h>
#include "support/BRInt.h"
#include "support/BRArray.h"
#include "ethereum/rlp/BRRlp.h"
#include "BREthereumLogic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ETHEREUM_HASH_BYTES    (256/8)

/**
 * An Ethereum 256-bit Keccak hash
 */
typedef struct {
    uint8_t bytes[ETHEREUM_HASH_BYTES];
} BREthereumHash;

#define EMPTY_HASH_INIT   ((const BREthereumHash) { \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0  \
})

#define HASH_INIT(s) ((BREthereumHash) { .bytes = {\
(_hexu((s)[ 0]) << 4) | _hexu((s)[ 1]), (_hexu((s)[ 2]) << 4) | _hexu((s)[ 3]),\
(_hexu((s)[ 4]) << 4) | _hexu((s)[ 5]), (_hexu((s)[ 6]) << 4) | _hexu((s)[ 7]),\
(_hexu((s)[ 8]) << 4) | _hexu((s)[ 9]), (_hexu((s)[10]) << 4) | _hexu((s)[11]),\
(_hexu((s)[12]) << 4) | _hexu((s)[13]), (_hexu((s)[14]) << 4) | _hexu((s)[15]),\
(_hexu((s)[16]) << 4) | _hexu((s)[17]), (_hexu((s)[18]) << 4) | _hexu((s)[19]),\
(_hexu((s)[20]) << 4) | _hexu((s)[21]), (_hexu((s)[22]) << 4) | _hexu((s)[23]),\
(_hexu((s)[24]) << 4) | _hexu((s)[25]), (_hexu((s)[26]) << 4) | _hexu((s)[27]),\
(_hexu((s)[28]) << 4) | _hexu((s)[29]), (_hexu((s)[30]) << 4) | _hexu((s)[31]),\
(_hexu((s)[32]) << 4) | _hexu((s)[33]), (_hexu((s)[34]) << 4) | _hexu((s)[35]),\
(_hexu((s)[36]) << 4) | _hexu((s)[37]), (_hexu((s)[38]) << 4) | _hexu((s)[39]),\
(_hexu((s)[40]) << 4) | _hexu((s)[41]), (_hexu((s)[42]) << 4) | _hexu((s)[43]),\
(_hexu((s)[44]) << 4) | _hexu((s)[45]), (_hexu((s)[46]) << 4) | _hexu((s)[47]),\
(_hexu((s)[48]) << 4) | _hexu((s)[49]), (_hexu((s)[50]) << 4) | _hexu((s)[51]),\
(_hexu((s)[52]) << 4) | _hexu((s)[53]), (_hexu((s)[54]) << 4) | _hexu((s)[55]),\
(_hexu((s)[56]) << 4) | _hexu((s)[57]), (_hexu((s)[58]) << 4) | _hexu((s)[59]),\
(_hexu((s)[60]) << 4) | _hexu((s)[61]), (_hexu((s)[62]) << 4) | _hexu((s)[63]) } })

/**
 * Create a Hash by converting from a hex-encoded string of a hash.  The string must
 * begin with '0x'.
 */
extern BREthereumHash
ethHashCreate (const char *string);

/**
 * Create an empty (all zeros) Hash
 */
extern BREthereumHash
ethHashCreateEmpty (void);

/**
 * Creata a Hash by computing it, using Keccak256, from a arbitrary data set
 */
extern BREthereumHash
ethHashCreateFromData (BRRlpData data);

/**
 * Return the hex-encoded string
 */
extern char *
ethHashAsString (BREthereumHash hash);

extern BREthereumHash
ethHashCopy(BREthereumHash hash);

extern BREthereumComparison
ethHashCompare(BREthereumHash hash1, BREthereumHash hash2);

extern BREthereumBoolean
ethHashEqual (BREthereumHash hash1, BREthereumHash hash2);

extern BRRlpItem
ethHashRlpEncode(BREthereumHash hash, BRRlpCoder coder);

extern BREthereumHash
ethHashRlpDecode (BRRlpItem item, BRRlpCoder coder);

extern BRRlpItem
ethHashEncodeList (BRArrayOf(BREthereumHash) hashes, BRRlpCoder coder);

// BRSet Support
inline static int
ethHashSetValue (const BREthereumHash *hash) {
    return ((UInt256 *) hash->bytes)->u32[0];
}

// BRSet Support
inline static int
ethHashSetEqual (const BREthereumHash *hash1,
                 const BREthereumHash *hash2) {
    return hash1 == hash2 || 0 == memcmp (hash1->bytes, hash2->bytes, ETHEREUM_HASH_BYTES);
}

//
// Hash String (0x prefaced)
//
typedef char BREthereumHashString[2 * ETHEREUM_HASH_BYTES + 3];

extern void
ethHashFillString (BREthereumHash hash,
                   BREthereumHashString string);

//
// Hash Array
//
extern BRArrayOf(BREthereumHash)
ethHashesCopy (BRArrayOf(BREthereumHash) hashes);

extern ssize_t
ethHashesIndex (BRArrayOf(BREthereumHash) hashes,
                BREthereumHash hash);

#ifdef __cplusplus
}
#endif

#endif /* BREthereumHash_h */
