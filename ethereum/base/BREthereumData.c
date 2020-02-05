//
//  BREthereumData.c
//  BRCore
//
//  Created by Ed Gamble on 10/3/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "ethereum/util/BRUtil.h" // hexEncodeCreate()
#include "BREthereumData.h"

/// MARK: - Data

extern BREthereumData
ethDataCreateFromBytes (size_t count, uint8_t *bytes, int takeBytes) {
    uint8_t *data = bytes;  // take the bytes...

    // ... oops, we can't, so copy.
    if (!takeBytes) {
        data = malloc (count);
        memcpy (data, bytes, count);
    }

    return (BREthereumData) { count, data };
}

extern BREthereumData
ethDataCreateFromRlpData (BRRlpData rlp,
                          int takeBytes) {
    return ethDataCreateFromBytes (rlp.bytesCount, rlp.bytes, takeBytes);
}

extern BREthereumData
ethDataCreateFromString (const char *string) {
    BREthereumData data;
    data.bytes = hexDecodeCreate(&data.count, string, strlen (string));
    return data;
}

extern BREthereumData
ethDataCopy (BREthereumData data) {
    return ethDataCreateFromBytes (data.count, data.bytes, 1);
}

extern void
ethDataRelease (BREthereumData data) {
    if (NULL != data.bytes) free (data.bytes);
}

extern char *
ethDataAsString (BREthereumData data) {
    return hexEncodeCreate (NULL, data.bytes, data.count);
}

extern BRRlpData
ethDataAsRlpData (BREthereumData data) {
    BRRlpData rlp = { data.count, malloc (data.count) };
    memcpy (rlp.bytes, data.bytes, data.count);
    return rlp;
}

/// MARK: - Hash Data Pair

struct BREthereumHashDataPairRecord {
    BREthereumHash hash;
    BREthereumData data;
};

extern BREthereumHashDataPair
ethHashDataPairCreate (BREthereumHash hash,
                       BREthereumData data) {
    BREthereumHashDataPair pair = malloc (sizeof (struct BREthereumHashDataPairRecord));

    pair->hash = hash;
    pair->data = data;

    return pair;
}

extern BREthereumHashDataPair
ethHashDataPairCreateFromString (const char *hashString,
                                 const char *dataString) {
    return ethHashDataPairCreate (ethHashCreate (hashString),
                                  ethDataCreateFromString (dataString));
}

extern void
ethHashDataPairRelease (BREthereumHashDataPair pair) {
    ethDataRelease (pair->data);
    free (pair);
}

extern BREthereumHash
ethHashDataPairGetHash (BREthereumHashDataPair pair) {
    return pair->hash;
}

// The hex-encoded hash.  You own this.
extern char *
ethHashDataPairGetHashAsString (BREthereumHashDataPair pair) {
    return ethHashAsString (pair->hash);
}

// YOU DO NOT OWN THE RETURNED DATA
extern BREthereumData
ethHashDataPairGetData (BREthereumHashDataPair pair) {
    return pair->data;
}

extern char *
ethHashDataPairGetDataAsString (BREthereumHashDataPair pair) {
    return ethDataAsString (pair->data);
}

extern void
ethHashDataPairExtractStrings (BREthereumHashDataPair pair,
                               char **hashAsString,
                               char **dataAsString) {
    if (NULL != hashAsString) *hashAsString = ethHashDataPairGetHashAsString (pair);
    if (NULL != dataAsString) *dataAsString = ethHashDataPairGetDataAsString (pair);
}

extern size_t
ethHashDataPairHashValue (const void *t)
{
    return ethHashSetValue(&((BREthereumHashDataPair) t)->hash);
}


extern int
ethHashDataPairHashEqual (const void *t1,
                          const void *t2) {
    return t1 == t2 || ethHashSetEqual (&((BREthereumHashDataPair) t1)->hash,
                                        &((BREthereumHashDataPair) t2)->hash);
}

/// MARK: - Hash Data Pair Set
extern BRSetOf (BREthereumHashDataPair)
ethHashDataPairSetCreateEmpty(size_t count) {
    return BRSetNew (ethHashDataPairHashValue,
                     ethHashDataPairHashEqual,
                     count);
}

extern BRSetOf (BREthereumHashDataPair)
ethHashDataPairSetCreate (BRArrayOf(BREthereumHashDataPair) pairs) {
    size_t count = array_count (pairs);
    BRSetOf(BREthereumHashDataPair) set = ethHashDataPairSetCreateEmpty (count);

    for (size_t index = 0; index < count; index++)
        BRSetAdd (set, pairs[index]);

    array_free (pairs);
    return set;
}

extern void
ethHashDataPairSetRelease (BRSetOf (BREthereumHashDataPair) set) {
    size_t itemsCount = BRSetCount (set);
    void  *itemsAll [itemsCount];

    BRSetAll (set, itemsAll,  itemsCount);
    for (size_t index = 0; index < itemsCount; index++)
        ethHashDataPairRelease ((BREthereumHashDataPair) itemsAll[index]);
    
    BRSetClear (set);
    BRSetFree  (set);
}

extern void
ethHashDataPairAdd (BRSetOf(BREthereumHashDataPair) set,
                    const char *hash,
                    const char *data) {
    BRSetAdd (set, ethHashDataPairCreateFromString(hash, data));
}
