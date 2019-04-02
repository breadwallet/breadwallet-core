//
//  BREthereumData.c
//  BRCore
//
//  Created by Ed Gamble on 10/3/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "ethereum/util/BRUtil.h" // encodeHexCreate()
#include "BREthereumData.h"

/// MARK: - Data

extern BREthereumData
dataCreateFromBytes (size_t count, uint8_t *bytes, int takeBytes) {
    uint8_t *data = bytes;  // take the bytes...

    // ... oops, we can't, so copy.
    if (!takeBytes) {
        data = malloc (count);
        memcpy (data, bytes, count);
    }

    return (BREthereumData) { count, data };
}

extern BREthereumData
dataCreateFromRlpData (BRRlpData rlp,
                       int takeBytes) {
    return dataCreateFromBytes (rlp.bytesCount, rlp.bytes, takeBytes);
}

extern BREthereumData
dataCreateFromString (const char *string) {
    BREthereumData data;
    data.bytes = decodeHexCreate(&data.count, string, strlen (string));
    return data;
}

extern BREthereumData
dataCopy (BREthereumData data) {
    return dataCreateFromBytes (data.count, data.bytes, 1);
}

extern void
dataRelease (BREthereumData data) {
    if (NULL != data.bytes) free (data.bytes);
}

extern char *
dataAsString (BREthereumData data) {
    return encodeHexCreate (NULL, data.bytes, data.count);
}

extern BRRlpData
dataAsRlpData (BREthereumData data) {
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
hashDataPairCreate (BREthereumHash hash,
                    BREthereumData data) {
    BREthereumHashDataPair pair = malloc (sizeof (struct BREthereumHashDataPairRecord));

    pair->hash = hash;
    pair->data = data;

    return pair;
}

extern BREthereumHashDataPair
hashDataPairCreateFromString (const char *hashString,
                              const char *dataString) {
    return hashDataPairCreate (hashCreate (hashString),
                               dataCreateFromString (dataString));
}

extern void
hashDataPairRelease (BREthereumHashDataPair pair) {
    dataRelease (pair->data);
    free (pair);
}

extern BREthereumHash
hashDataPairGetHash (BREthereumHashDataPair pair) {
    return pair->hash;
}

// The hex-encoded hash.  You own this.
extern char *
hashDataPairGetHashAsString (BREthereumHashDataPair pair) {
    return hashAsString (pair->hash);
}

// YOU DO NOT OWN THE RETURNED DATA
extern BREthereumData
hashDataPairGetData (BREthereumHashDataPair pair) {
    return pair->data;
}

extern char *
hashDataPairGetDataAsString (BREthereumHashDataPair pair) {
    return dataAsString (pair->data);
}

extern void
hashDataPairExtractStrings (BREthereumHashDataPair pair,
                            char **hashAsString,
                            char **dataAsString) {
    if (NULL != hashAsString) *hashAsString = hashDataPairGetHashAsString (pair);
    if (NULL != dataAsString) *dataAsString = hashDataPairGetDataAsString (pair);
}

extern size_t
hashDataPairHashValue (const void *t)
{
    return hashSetValue(&((BREthereumHashDataPair) t)->hash);
}


extern int
hashDataPairHashEqual (const void *t1,
                       const void *t2) {
    return t1 == t2 || hashSetEqual (&((BREthereumHashDataPair) t1)->hash,
                                     &((BREthereumHashDataPair) t2)->hash);
}

/// MARK: - Hash Data Pair Set
extern BRSetOf (BREthereumHashDataPair)
hashDataPairSetCreateEmpty(size_t count) {
    return BRSetNew (hashDataPairHashValue,
                     hashDataPairHashEqual,
                     count);
}

extern BRSetOf (BREthereumHashDataPair)
hashDataPairSetCreate (BRArrayOf(BREthereumHashDataPair) pairs) {
    size_t count = array_count (pairs);
    BRSetOf(BREthereumHashDataPair) set = hashDataPairSetCreateEmpty (count);

    for (size_t index = 0; index < count; index++)
        BRSetAdd (set, pairs[index]);

    array_free (pairs);
    return set;
}

extern void
hashDataPairSetRelease (BRSetOf (BREthereumHashDataPair) set) {
    size_t itemsCount = BRSetCount (set);
    void  *itemsAll [itemsCount];

    BRSetAll (set, itemsAll,  itemsCount);
    for (size_t index = 0; index < itemsCount; index++)
        hashDataPairRelease ((BREthereumHashDataPair) itemsAll[index]);
    
    BRSetClear (set);
    BRSetFree  (set);
}

extern void
hashDataPairAdd (BRSetOf(BREthereumHashDataPair) set,
                 const char *hash,
                 const char *data) {
    BRSetAdd (set, hashDataPairCreateFromString(hash, data));
}
