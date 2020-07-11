//
//  BREthereumData.h
//  BRCore
//
//  Created by Ed Gamble on 10/3/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

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
ethDataCreateFromBytes (size_t count,
                        uint8_t *bytes,
                        int takeBytes);

extern BREthereumData
ethDataCreateFromRlpData (BRRlpData rlp,
                          int takeBytes);

extern BREthereumData // invert dataAsString()
ethDataCreateFromString (const char *string) ;

extern BREthereumData
ethDataCopy (BREthereumData data);

extern void
ethDataRelease (BREthereumData data);

// hex-encoded data; you own this.
extern char *
ethDataAsString (BREthereumData data);

extern BRRlpData
ethDataAsRlpData (BREthereumData data);

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
 * on `ethHashDataPairRelease()` the `data` will itself be released.
 *
 * @param hash the hash
 * @param data the data
 * @return the hash-data-pair
 */
extern BREthereumHashDataPair
ethHashDataPairCreate (BREthereumHash hash,
                       BREthereumData data);

extern BREthereumHashDataPair
ethHashDataPairCreateFromString (const char *hashString,
                                 const char *dataString);

extern void
ethHashDataPairRelease (BREthereumHashDataPair pair);

static inline void
ethHashDataPairReleaseForSet (void *ignore, void *item) {
    ethHashDataPairRelease((BREthereumHashDataPair) item);
}

extern BREthereumHash
ethHashDataPairGetHash (BREthereumHashDataPair pair);

// The hex-encoded hash.  You own this.
extern char *
ethHashDataPairGetHashAsString (BREthereumHashDataPair pair);

// YOU DO NOT OWN THE RETURNED DATA
extern BREthereumData
ethHashDataPairGetData (BREthereumHashDataPair pair);

// The hex-encoded data.  You own this.
extern char *
ethHashDataPairGetDataAsString (BREthereumHashDataPair pair);

extern void
ethHashDataPairExtractStrings (BREthereumHashDataPair pair,
                               char **hashAsString,
                               char **dataAsString);

extern size_t
ethHashDataPairHashValue (const void *t);

extern int
ethHashDataPairHashEqual (const void *t1,
                          const void *t2);

/// MARK: - Hash Data Pair Set

extern BRSetOf (BREthereumHashDataPair)
ethHashDataPairSetCreateEmpty (size_t count);

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
ethHashDataPairSetCreate (BRArrayOf(BREthereumHashDataPair) pairs);;

extern void
ethHashDataPairSetRelease (BRSetOf (BREthereumHashDataPair) set);

extern void
ethHashDataPairAdd (BRSetOf(BREthereumHashDataPair) set,
                    const char *hash,
                    const char *data);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Data_H */
