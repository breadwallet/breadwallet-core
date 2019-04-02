//
//  BREthereumBloomFilter.h
//  BRCore
//
//  Created by Ed Gamble on 5/10/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Bloom_Filter_h
#define BR_Ethereum_Bloom_Filter_h

#include "ethereum/base/BREthereumBase.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ETHEREUM_BLOOM_FILTER_BITS 2048
#define ETHEREUM_BLOOM_FILTER_BYTES   (ETHEREUM_BLOOM_FILTER_BITS / 8)

/**
 * An Etereum Bloom Filter is a 2048-bit 'fuzzy' representation of one or more addresses.
 */
typedef struct {
    uint8_t bytes[ETHEREUM_BLOOM_FILTER_BYTES];
} BREthereumBloomFilter;

#define EMPTY_BLOOM_FILTER_INIT   ((const  BREthereumBloomFilter) { \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, \
})

/**
 * Create an empty BloomFilter
 */
extern BREthereumBloomFilter
bloomFilterCreateEmpty (void);

/**
 * Create a BloomFilter from `hash`
 */
extern BREthereumBloomFilter
bloomFilterCreateHash (const BREthereumHash hash);

/**
 * Create a BloomFilter from `data` - computes the hash of `data`
 */
extern BREthereumBloomFilter
bloomFilterCreateData (const BRRlpData data);

/**
 * Create a BloomFilter from `address` - computes the hash of `address`
 */
extern BREthereumBloomFilter
bloomFilterCreateAddress (const BREthereumAddress address);

/**
 * Create a BloomFilter from a hex-encoded, non-0x-prefaced string.
 */
extern BREthereumBloomFilter
bloomFilterCreateString (const char *string);

extern BREthereumBloomFilter
bloomFilterOr (const BREthereumBloomFilter filter1, const BREthereumBloomFilter filter2);

extern void
bloomFilterOrInPlace (BREthereumBloomFilter filter1, const BREthereumBloomFilter filter2);

extern BREthereumBoolean
bloomFilterEqual (const BREthereumBloomFilter filter1, const BREthereumBloomFilter filter2);

/**
 * Check if `other` is contained in `filter`.  Typically `filter` would be the bloom filter
 * for a block header and `other` would be an address (source,target,contract) of
 * interest.
 *
 * @parameter filter
 *
 * @parameter other
 *
 * @returns TRUE if `other` matches `filter`; otherwise FALSE
 */
extern BREthereumBoolean
bloomFilterMatch (const BREthereumBloomFilter filter, const BREthereumBloomFilter other);

extern BRRlpItem
bloomFilterRlpEncode(BREthereumBloomFilter filter, BRRlpCoder coder);

extern BREthereumBloomFilter
bloomFilterRlpDecode (BRRlpItem item, BRRlpCoder coder);

/**
 * Return a hex-encode string representation of `filter`.
 */
extern char *
bloomFilterAsString (BREthereumBloomFilter filter);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Bloom_Filter_h */
