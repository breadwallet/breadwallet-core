//
//  BREthereumHash.c
//  BRCore
//
//  Created by Ed Gamble on 5/9/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
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
ethHashCreate (const char *string) {
    if (NULL == string || '\0' == string[0] || 0 == strcmp (string, "0x")) return ethHashCreateEmpty();

    assert (0 == strncmp (string, "0x", 2)
            && (2 + 2 * ETHEREUM_HASH_BYTES) == strlen (string));

    BREthereumHash hash;
    hexDecode(hash.bytes, ETHEREUM_HASH_BYTES, &string[2], 2 * ETHEREUM_HASH_BYTES);
    return hash;
}

/**
 * Create an empty (all zeros) Hash
 */
extern BREthereumHash
ethHashCreateEmpty (void) {
    return emptyHash;
}

/**
 * Creata a Hash by computing it from a arbitrary data set (using Keccak256)
 */
extern BREthereumHash
ethHashCreateFromData (BRRlpData data) {
    BREthereumHash hash;
    BRKeccak256(hash.bytes, data.bytes, data.bytesCount);
    return hash;
}

/**
 * Return the hex-encoded string
 */
extern char *
ethHashAsString (BREthereumHash hash) {
    char result [2 + 2 * ETHEREUM_HASH_BYTES + 1];
    result[0] = '0';
    result[1] = 'x';
    hexEncode(&result[2], 2 * ETHEREUM_HASH_BYTES + 1, hash.bytes, ETHEREUM_HASH_BYTES);
    return strdup (result);
}

extern BREthereumHash
ethHashCopy(BREthereumHash hash) {
    return hash;
}

extern BREthereumComparison
ethHashCompare(BREthereumHash hash1, BREthereumHash hash2) {
    for (int i = 0; i < ETHEREUM_HASH_BYTES; i++) {
        if (hash1.bytes[i] > hash2.bytes[i]) return ETHEREUM_COMPARISON_GT;
        else if (hash1.bytes[i] < hash2.bytes[i]) return ETHEREUM_COMPARISON_LT;
    }
    return ETHEREUM_COMPARISON_EQ;
}

extern BREthereumBoolean
ethHashEqual (BREthereumHash hash1, BREthereumHash hash2) {
    return AS_ETHEREUM_BOOLEAN (0 == memcmp (hash1.bytes, hash2.bytes, ETHEREUM_HASH_BYTES));
}

extern BRRlpItem
ethHashRlpEncode(BREthereumHash hash, BRRlpCoder coder) {
    return rlpEncodeBytes(coder, hash.bytes, ETHEREUM_HASH_BYTES);
}

extern BREthereumHash
ethHashRlpDecode (BRRlpItem item, BRRlpCoder coder) {
    BREthereumHash hash;

    BRRlpData data = rlpDecodeBytes(coder, item);
    assert (ETHEREUM_HASH_BYTES == data.bytesCount);

    memcpy (hash.bytes, data.bytes, ETHEREUM_HASH_BYTES);
    rlpDataRelease(data);

    return hash;
}

extern BRRlpItem
ethHashEncodeList (BRArrayOf (BREthereumHash) hashes, BRRlpCoder coder) {
    size_t itemCount = array_count(hashes);
    BRRlpItem items[itemCount];
    for (size_t index = 0; index < itemCount; index++)
        items[index] = ethHashRlpEncode(hashes[index], coder);
    return rlpEncodeListItems (coder, items, itemCount);
}

extern void
ethHashFillString (BREthereumHash hash,
                   BREthereumHashString string) {
    string[0] = '0';
    string[1] = 'x';
    hexEncode(&string[2], 2 * ETHEREUM_HASH_BYTES + 1, hash.bytes, ETHEREUM_HASH_BYTES);
}

extern BRArrayOf(BREthereumHash)
ethHashesCopy (BRArrayOf(BREthereumHash) hashes) {
    BRArrayOf(BREthereumHash) result;
    array_new (result, array_count(hashes));
    array_add_array (result, hashes, array_count(hashes));
    return result;
}

extern ssize_t
ethHashesIndex (BRArrayOf(BREthereumHash) hashes,
                BREthereumHash hash) {
    for (size_t index = 0; index < array_count(hashes); index++)
        if (ETHEREUM_BOOLEAN_IS_TRUE (ethHashEqual (hash, hashes[index])))
            return index;
    return -1;
}
