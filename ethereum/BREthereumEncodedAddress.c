//
//  BREthereumEncodedAddress
//  breadwallet-core Ethereum
//
//  Created by ebg on 6/8/18.
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

#include <assert.h>
#include "BREthereumEncodedAddress.h"

//
// Address
//
#if 0
/**
 * Two address types - explicitly provided or derived from BIP44
 */
typedef enum {
    ADDRESS_PROVIDED,   // target,
    ADDRESS_DERIVED,    // from BIP44
} BREthereumAddressType;

/**
 * An EthereumAddress is as '0x'-prefixed, hex-encoded string with an overall lenght of 42
 * characters.  Addresses can be explicitly provided - such as with a 'send to' addresses; or can
 * be derived using BIP44 scheme - such as with internal addresses.
 */
struct BREthereumEncodedAddressRecord {
    BREthereumAddress raw;

    /**
     * The 'official' ethereum address string for (the external representation of) this
     * BREthereum address.
     *
     * THIS IS NOT A SIMPLE STRING; this is a hex encoded (with encodeHex) string prefixed with
     * "0x".  Generally, when using this string, for example when RLP encoding, one needs to
     * convert back to the byte array (use rlpEncodeItemHexString())
     */
    char string[43];    // '0x' + <40 chars> + '\0'

    /**
     * Identify the type of this address record - created with a provided string or
     * with a provided publicKey.
     */
    BREthereumAddressType type;

    /**
     * The public key.  This started out as a BIP44 264 bits (65 bytes) array with a value of
     * 0x04 at byte 0; we strip off that first byte and are left with 64.  Go figure.
     */
    uint8_t publicKey [64];  // BIP44: 'Master Public Key 'M' (264 bits) - 8

    /**
     * The BIP-44 Index used for this key.
     */
    uint32_t index;

    /**
     * The NEXT nonce value
     */
    uint64_t nonce;
};

static struct BREthereumEncodedAddressRecord emptyAddressRecord;
const BREthereumEncodedAddress emptyAddress = &emptyAddressRecord;

extern BREthereumEncodedAddress
createAddress (const char *string) {
    if (ETHEREUM_BOOLEAN_IS_FALSE(validateAddressString(string))) return NULL;

    BREthereumEncodedAddress address = malloc (sizeof (struct BREthereumEncodedAddressRecord));

    address->type = ADDRESS_PROVIDED;
    address->nonce = 0;
    address->raw = addressRawCreate(string);
    strncpy (address->string, string, 42);
    address->string[42] = '\0';

    return address;
}

extern BREthereumBoolean
validateAddressString(const char *string) {
    return 42 == strlen(string)
           && '0' == string[0]
           && 'x' == string[1]
           && encodeHexValidate (&string[2])
           ? ETHEREUM_BOOLEAN_TRUE
           : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumEncodedAddress
createAddressRaw (BREthereumAddress raw) {
    BREthereumEncodedAddress address = malloc (sizeof (struct BREthereumEncodedAddressRecord));

    address->type = ADDRESS_PROVIDED;
    address->nonce = 0;
    address->raw = raw;
    address->string[0] = '0';
    address->string[1] = 'x';
    encodeHex(&address->string[2], 40 + 1, raw.bytes, 20);

    return address;
}

extern BREthereumAddress
addressGetRawAddress (BREthereumEncodedAddress address) {
    return address->raw;
}

extern void
addressFree (BREthereumEncodedAddress address) {
    free (address);
}

//extern uint32_t
//addressGetIndex (BREthereumEncodedAddress address) {
//    return address->index;
//}
//
//extern uint64_t
//addressGetNonce(BREthereumEncodedAddress address) {
//    return address->nonce;
//}
//
//private_extern void
//addressSetNonce(BREthereumEncodedAddress address,
//                uint64_t nonce,
//                BREthereumBoolean force) {
//    if (ETHEREUM_BOOLEAN_IS_TRUE(force) || nonce > address->nonce)
//        address->nonce = nonce;
//}
//
//private_extern uint64_t
//addressGetThenIncrementNonce(BREthereumEncodedAddress address) {
//    return address->nonce++;
//}

/**
 * Create an address given a 65 byte publicKey (derived from a BIP-44 public key).
 *
 * Details: publicKey[0] must be '0x04';
 *
 * @param publicKey
 * @return
 */
extern BREthereumEncodedAddress
createAddressDerived (const BRKey *key, uint32_t index) {
    BREthereumEncodedAddress address = malloc (sizeof (struct BREthereumEncodedAddressRecord));

    address->type = ADDRESS_DERIVED;  // painfully
    address->nonce = 0;
    address->index = index;

    // Seriously???
    //
    // https://kobl.one/blog/create-full-ethereum-keypair-and-address/#derive-the-ethereum-address-from-the-public-key
    //
    // "The public key is what we need in order to derive its Ethereum address. Every EC public key
    // begins with the 0x04 prefix before giving the location of the two point on the curve. You
    // should remove this leading 0x04 byte in order to hash it correctly. ...

    assert (key->pubKey[0] == 0x04);

    // Strip off byte 0
    memcpy(address->publicKey, &key->pubKey[1], sizeof (address->publicKey));


    address->raw = addressRawCreateKey(key);
    char *string = addressRawGetEncodedString(address->raw, 1);
    memcpy (address->string, string, 42);
    address->string[42] = '\0';
    free (string);

    return address;
}

extern BREthereumBoolean
addressHasString (BREthereumEncodedAddress address,
                  const char *string) {
    return (0 == strcasecmp(string, address->string)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumBoolean
addressEqual (BREthereumEncodedAddress a1, BREthereumEncodedAddress a2) {
    return (0 == strcmp (a1->string, a2->string)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern char *
addressAsString (BREthereumEncodedAddress address) {
    return strndup (address->string, 43);
}

extern BRKey // 65 bytes
addressGetPublicKey (BREthereumEncodedAddress address) {
    BRKey result;
    BRKeyClean(&result);

    result.pubKey[0] = 0x04;
    memcpy (&result.pubKey[1], address->publicKey, sizeof (address->publicKey));

    return result;
}

#if defined (DEBUG)
extern const char *
addressPublicKeyAsString (BREthereumEncodedAddress address, int compressed) {
    // The byte array at address->publicKey has the '04' 'uncompressed' prefix removed.  Thus
    // the value in publicKey is uncompressed and 64 bytes.  As a string, this result will have
    // an 0x0<n> prefix where 'n' is in { 4: uncompressed, 2: compressed even, 3: compressed odd }.

    // Default, uncompressed
    char *prefix = "0x04";
    size_t sourceLen = sizeof (address->publicKey);           // 64 bytes: { x y }

    if (compressed) {
        sourceLen /= 2;  // use 'x'; skip 'y'
        prefix = (0 == address->publicKey[63] % 2 ? "0x02" : "0x03");
    }

    char *result = malloc (4 + 2 * sourceLen + 1);
    strcpy (result, prefix);  // encode properly...
    encodeHex(&result[4], 2 * sourceLen + 1, address->publicKey, sourceLen);

    return result;
}
#endif

extern BRRlpItem
addressRlpEncode (BREthereumEncodedAddress address, BRRlpCoder coder) {
    return rlpEncodeItemHexString(coder, address->string);
}

extern BREthereumEncodedAddress
addressRlpDecode (BRRlpItem item, BRRlpCoder coder) {
    return createAddress(rlpDecodeItemHexString (coder, item, "0x"));
}
#endif
