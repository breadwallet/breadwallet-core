//
//  BREthereumHash.c
//  BRCore
//
//  Created by Ed Gamble on 5/9/18.
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "BRCrypto.h"
#include "../util/BRUtil.h"
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
