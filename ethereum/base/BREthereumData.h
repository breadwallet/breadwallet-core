//
//  BREthereumData.h
//  BRCore
//
//  Created by Ed Gamble on 10/3/18.
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

#ifndef BR_Ethereum_Data_H
#define BR_Ethereum_Data_H

#include "support/BRArray.h"
#include "support/BRSet.h"
#include "ethereum/rlp/BRRlp.h"
#include "BREthereumHash.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK: - Data

/**
 * Ethereum Data is an pair of { count, bytes} representing an arbitrary array of bytes.
 */
typedef struct {
    size_t count;
    uint8_t *bytes;
} BREthereumData;

extern BREthereumData
dataCreateFromBytes (size_t count,
                     uint8_t *bytes,
                     int takeBytes);

extern BREthereumData
dataCreateFromRlpData (BRRlpData rlp,
                       int takeBytes);

extern BREthereumData // invert dataAsString()
dataCreateFromString (const char *string) ;

extern BREthereumData
dataCopy (BREthereumData data);

extern void
dataRelease (BREthereumData data);

// hex-encoded data; you own this.
extern char *
dataAsString (BREthereumData data);

extern BRRlpData
dataAsRlpData (BREthereumData data);

/// MARK: - Hash Data Pair

/**
 * An Ethereum Hash Data Pair holds {Hash, Data} where the hash can be considered a unique
 * identifier for the Data
 */
typedef struct BREthereumHashDataPairRecord *BREthereumHashDataPair;

/**
 * Create a BREthereumHashDataPair
 *
 * @note Takes ownership of `data` which must have been created with `takeBytes` (because
 * on `hashDataPairRelease()` the `data` will itself be released.
 *
 * @param hash the hash
 * @param data the data
 * @return the hash-data-pair
 */
extern BREthereumHashDataPair
hashDataPairCreate (BREthereumHash hash,
                    BREthereumData data);

extern BREthereumHashDataPair
hashDataPairCreateFromString (const char *hashString,
                              const char *dataString);

extern void
hashDataPairRelease (BREthereumHashDataPair pair);

static inline void
hashDataPairReleaseForSet (void *ignore, void *item) {
    hashDataPairRelease((BREthereumHashDataPair) item);
}

extern BREthereumHash
hashDataPairGetHash (BREthereumHashDataPair pair);

// The hex-encoded hash.  You own this.
extern char *
hashDataPairGetHashAsString (BREthereumHashDataPair pair);

// YOU DO NOT OWN THE RETURNED DATA
extern BREthereumData
hashDataPairGetData (BREthereumHashDataPair pair);

// The hex-encoded data.  You own this.
extern char *
hashDataPairGetDataAsString (BREthereumHashDataPair pair);

extern void
hashDataPairExtractStrings (BREthereumHashDataPair pair,
                            char **hashAsString,
                            char **dataAsString);

extern size_t
hashDataPairHashValue (const void *t);

extern int
hashDataPairHashEqual (const void *t1,
                       const void *t2);

/// MARK: - Hash Data Pair Set

extern BRSetOf (BREthereumHashDataPair)
hashDataPairSetCreateEmpty (size_t count);

/**
 * Create a BRSet of BREthereumHashDataPair.
 *
 * @note This function takes ownership of `pairs` (that is, owns the individual
 * pair items and frees the pairs array.
 *
 * @param BREthereumHashDataPair array of pairs to add to the set.
 * @return a BRSET of BREthereumHashDataPair
 */
extern BRSetOf (BREthereumHashDataPair)
hashDataPairSetCreate (BRArrayOf(BREthereumHashDataPair) pairs);;

extern void
hashDataPairSetRelease (BRSetOf (BREthereumHashDataPair) set);

extern void
hashDataPairAdd (BRSetOf(BREthereumHashDataPair) set,
                 const char *hash,
                 const char *data);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Data_H */
