//
//  BREthereumAddress.c
//  BRCore
//
//  Created by Ed Gamble on 5/17/18.
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "BRCrypto.h"
#include "BREthereumBase.h"
#include "BREthereumAddress.h"

//
// Address Raw
//
extern BREthereumAddress
addressRawCreate (const char *address) {
    BREthereumAddress raw;
    if (0 == strncmp ("0x", address, 2)) address = &address[2];
    decodeHex(raw.bytes, sizeof(raw.bytes), address, strlen(address));
    return raw;
}

extern BREthereumAddress
addressRawCreateKey (const BRKey *key) {
    BREthereumAddress address;

    //assert ( 0 == key->compressed);
    //assert (65 == BRKeyPubKey(key, NULL, 0));

    // We interrupt your regularly scheduled programming...

    // "Use any method you like to get it in the form of an hexadecimal string
    // "The <pub file> now contains the hexadecimal value of the public key without the 0x04 prefix.

    // "An Ethereum address is made of 20 bytes (40 hex characters long), it is commonly
    // represented by adding the 0x prefix. In order to derive it, one should take the keccak-256
    // hash of the hexadecimal form of a public key, then keep only the last 20 bytes (aka get
    // rid of the first 12 bytes).
    //
    // "Simply pass the file containing the public key in hexadecimal format to the keccak-256sum
    // command. Do not forget to use the ‘-x’ option in order to interpret it as hexadecimal and
    // not a simple string.
    //
    // WTF is a 'simple string'.  Seriously??

    // Back to our regularly scheduled programming...
    //
    // We'll assume our BRKeccak256 takes an array of bytes (sure, the argument is void*); NOT
    // a hexadecimal format of a 0x04 stripped public key...

    uint8_t hash[32];
    BRKeccak256(hash, &key->pubKey[1], sizeof (key->pubKey) - 1);

    memcpy (address.bytes, &hash[12], 20);

    return address;
}

extern char *
addressRawGetEncodedString (BREthereumAddress address, int useChecksum) {
    char *string = calloc (1, 43);


    // Fill in string
    string[0] = '0';
    string[1] = 'x';

    // Offset '2' into address->string and account for the '\0' terminator.
    encodeHex(&string[2], 40 + 1, address.bytes, 20);

    if (!useChecksum) return string;

    // And now the 'checksum after thought'

    // https://ethereum.stackexchange.com/a/19048/33128
    //
    // Ethereum wallet addresses are in hex [0-9A-F]*. While the address itself is case-insensitive
    // (A is the same as a to the network), the case sensitivity is used as a (optional) checksum.
    // It was built as an after-thought to an addressing scheme that lacked basic checksum
    // validation.  https://github.com/ethereum/EIPs/issues/55#issuecomment-187159063
    //
    // The checksum works like so:
    //
    // 1) lowercase address and remove 0x prefix
    // 2) sha3 hash result from #1
    // 3) change nth letter of address according to the nth letter of the hash:
    //      0,1,2,3,4,5,6,7 → Lowercase
    //      8, 9, a, b, c, d, e, f → Uppercase
    //
    // So, you sha3 hash the address, and look at each Nth character of the sha result. If it's 7
    // or below, the Nth character in the address is lowercase. If it is 8 or above, that character
    // is uppercase.

    // We'll skip it - unless somebody requests it.

    // PaperKey: boring head harsh green empty clip fatal typical found crane dinner timber
    //  Address: 0xa9de3dbd7d561e67527bc1ecb025c59d53b9f7ef
    //   Result: 0xa9de3dbD7d561e67527bC1Ecb025c59D53b9F7Ef
    //
    //        > web3.toChecksumAddress("0xa9de3dbd7d561e67527bc1ecb025c59d53b9f7ef")
    //          "0xa9de3dbD7d561e67527bC1Ecb025c59D53b9F7Ef"
    //
    //        > web3.sha3("a9de3dbd7d561e67527bc1ecb025c59d53b9f7ef")
    //          "0x6540e229f74514b83dd4a29553c029ad7b31c882df256a8c5222802c1b9b78d9"

    // We'll checksum address->string but while avoiding the '0x' prefix
    uint8_t hash[32];
    char *checksumAddr = &string[2];
    size_t checksumAddrLen = strlen(checksumAddr);
    assert (checksumAddrLen < 2 * sizeof(hash));

    // Ethereum 'SHA3' is actually Keccak256
    BRKeccak256(hash, checksumAddr, checksumAddrLen);

    for (int i = 0; i < checksumAddrLen; i++) {
        // We should hex-encode the hash and then look character by character.  Instead
        // we'll extract 4 bits as the upper or lower nibble and compare to 8.  This is the
        // same extracting that encodeHex performs, ultimately.
        int value = 0x0f & (hash[i / 2] >> ((0 == i % 2) ? 4 : 0));
        checksumAddr[i] = (value < 8
                           ? (char) tolower(checksumAddr[i])
                           : (char) toupper(checksumAddr[i]));
    }
    return string;
}

extern BREthereumAddress
addressRawRlpDecode (BRRlpItem item, BRRlpCoder coder) {
    BREthereumAddress address;

    BRRlpData data = rlpDecodeItemBytes(coder, item);
    assert (20 == data.bytesCount);

    memcpy (address.bytes, data.bytes, 20);
    return address;
}

extern BRRlpItem
addressRawRlpEncode(BREthereumAddress address,
                    BRRlpCoder coder) {
    return rlpEncodeItemBytes(coder, address.bytes, 20);
}

extern BREthereumBoolean
addressRawEqual (BREthereumAddress address1,
                 BREthereumAddress address2) {
    return (0 == memcmp(address1.bytes, address2.bytes, 20)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

//
// Address
//

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

extern uint32_t
addressGetIndex (BREthereumEncodedAddress address) {
    return address->index;
}

extern uint64_t
addressGetNonce(BREthereumEncodedAddress address) {
    return address->nonce;
}

private_extern void
addressSetNonce(BREthereumEncodedAddress address,
                uint64_t nonce) {
    address->nonce = nonce;
}

private_extern uint64_t
addressGetThenIncrementNonce(BREthereumEncodedAddress address) {
    return address->nonce++;
}

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

