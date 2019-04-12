//
//  BREthereumAddress.c
//  BRCore
//
//  Created by Ed Gamble on 5/17/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "support/BRCrypto.h"
#include "BREthereumBase.h"
#include "BREthereumAddress.h"

//
// Address Raw
//
extern BREthereumAddress
addressCreate (const char *address) {
    if (ETHEREUM_BOOLEAN_IS_FALSE(addressValidateString(address))) return EMPTY_ADDRESS_INIT;

    BREthereumAddress raw;
    if (0 == strncmp ("0x", address, 2)) address = &address[2];
    decodeHex(raw.bytes, sizeof(raw.bytes), address, strlen(address));
    return raw;
}

extern BREthereumBoolean
addressValidateString(const char *string) {
    return 42 == strlen(string)
           && '0' == string[0]
           && 'x' == string[1]
           && encodeHexValidate (&string[2])
           ? ETHEREUM_BOOLEAN_TRUE
           : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumAddress
addressCreateKey (const BRKey *key) {
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
addressGetEncodedString (BREthereumAddress address, int useChecksum) {
    char *string = calloc (1, ADDRESS_ENCODED_CHARS);
    addressFillEncodedString(address, useChecksum, string);
    return string;
}

extern void
addressFillEncodedString (BREthereumAddress address,
                          int useChecksum,
                          char *string) {

    // Fill in string
    string[0] = '0';
    string[1] = 'x';

    // Offset '2' into address->string and account for the '\0' terminator.
    encodeHex(&string[2], 40 + 1, address.bytes, 20);

    if (!useChecksum) return;

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
}

extern BREthereumHash
addressGetHash (BREthereumAddress address) {
    BRRlpData data = { 20, address.bytes };
    return hashCreateFromData(data);
}

extern BREthereumAddress
addressRlpDecode (BRRlpItem item, BRRlpCoder coder) {
    BREthereumAddress address = EMPTY_ADDRESS_INIT;

    BRRlpData data = rlpDecodeBytes(coder, item);
    if (0 != data.bytesCount) {
        assert (20 == data.bytesCount);
        memcpy (address.bytes, data.bytes, 20);
    }

    rlpDataRelease(data);
    return address;
}

extern BRRlpItem
addressRlpEncode(BREthereumAddress address,
                 BRRlpCoder coder) {
    return rlpEncodeBytes(coder, address.bytes, 20);
}

extern BREthereumBoolean
addressEqual (BREthereumAddress address1,
              BREthereumAddress address2) {
    return (0 == memcmp(address1.bytes, address2.bytes, 20)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

