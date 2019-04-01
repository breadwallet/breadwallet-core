//
//  BREthereumHash.c
//  BRCore
//
//  Created by Ed Gamble on 5/9/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "support/BRCrypto.h"
#include "ethereum/util/BRUtil.h"
#include "BREthereumHash.h"

static BREthereumHash emptyHash;

/**
 * Create a Hash by converting from a hex-encoded string of a hash.  The string must
 * begin with '0x'.
 */
extern BREthereumHash
hashCreate (const char *string) {
    if (NULL == string || '\0' == string[0] || 0 == strcmp (string, "0x")) return hashCreateEmpty();

    assert (0 == strncmp (string, "0x", 2)
            && (2 + 2 * ETHEREUM_HASH_BYTES) == strlen (string));

    BREthereumHash hash;
    decodeHex(hash.bytes, ETHEREUM_HASH_BYTES, &string[2], 2 * ETHEREUM_HASH_BYTES);
    return hash;
}

/**
 * Create an empty (all zeros) Hash
 */
extern BREthereumHash
hashCreateEmpty (void) {
    return emptyHash;
}

/**
 * Creata a Hash by computing it from a arbitrary data set (using Keccak256)
 */
extern BREthereumHash
hashCreateFromData (BRRlpData data) {
    BREthereumHash hash;
    BRKeccak256(hash.bytes, data.bytes, data.bytesCount);
    return hash;
}

/**
 * Return the hex-encoded string
 */
extern char *
hashAsString (BREthereumHash hash) {
    char result [2 + 2 * ETHEREUM_HASH_BYTES + 1];
    result[0] = '0';
    result[1] = 'x';
    encodeHex(&result[2], 2 * ETHEREUM_HASH_BYTES + 1, hash.bytes, ETHEREUM_HASH_BYTES);
    return strdup (result);
}

extern BREthereumHash
hashCopy(BREthereumHash hash) {
    return hash;
}

extern BREthereumComparison
hashCompare(BREthereumHash hash1, BREthereumHash hash2) {
    for (int i = 0; i < ETHEREUM_HASH_BYTES; i++) {
        if (hash1.bytes[i] > hash2.bytes[i]) return ETHEREUM_COMPARISON_GT;
        else if (hash1.bytes[i] < hash2.bytes[i]) return ETHEREUM_COMPARISON_LT;
    }
    return ETHEREUM_COMPARISON_EQ;
}

extern BREthereumBoolean
hashEqual (BREthereumHash hash1, BREthereumHash hash2) {
    return AS_ETHEREUM_BOOLEAN (0 == memcmp (hash1.bytes, hash2.bytes, ETHEREUM_HASH_BYTES));
}

extern BRRlpItem
hashRlpEncode(BREthereumHash hash, BRRlpCoder coder) {
    return rlpEncodeBytes(coder, hash.bytes, ETHEREUM_HASH_BYTES);
}

extern BREthereumHash
hashRlpDecode (BRRlpItem item, BRRlpCoder coder) {
    BREthereumHash hash;

    BRRlpData data = rlpDecodeBytes(coder, item);
    assert (ETHEREUM_HASH_BYTES == data.bytesCount);

    memcpy (hash.bytes, data.bytes, ETHEREUM_HASH_BYTES);
    rlpDataRelease(data);

    return hash;
}

extern BRRlpItem
hashEncodeList (BRArrayOf (BREthereumHash) hashes, BRRlpCoder coder) {
    size_t itemCount = array_count(hashes);
    BRRlpItem items[itemCount];
    for (size_t index = 0; index < itemCount; index++)
        items[index] = hashRlpEncode(hashes[index], coder);
    return rlpEncodeListItems (coder, items, itemCount);
}

extern void
hashFillString (BREthereumHash hash,
                BREthereumHashString string) {
    string[0] = '0';
    string[1] = 'x';
    encodeHex(&string[2], 2 * ETHEREUM_HASH_BYTES + 1, hash.bytes, ETHEREUM_HASH_BYTES);
}

extern BRArrayOf(BREthereumHash)
hashesCopy (BRArrayOf(BREthereumHash) hashes) {
    BRArrayOf(BREthereumHash) result;
    array_new (result, array_count(hashes));
    array_add_array (result, hashes, array_count(hashes));
    return result;
}

extern ssize_t
hashesIndex (BRArrayOf(BREthereumHash) hashes,
              BREthereumHash hash) {
    for (size_t index = 0; index < array_count(hashes); index++)
        if (ETHEREUM_BOOLEAN_IS_TRUE (hashEqual (hash, hashes[index])))
            return index;
    return -1;
}
