//
//  BRRippleAddress.c
//  Core
//
//  Created by Carl Cherry on Oct. 21, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include "BRRippleAddress.h"
#include "BRRippleBase58.h"
#include "support/BRCrypto.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

// A Ripple Address - 20 bytes
#define ADDRESS_BYTES   (20)

struct BRRippleAddressRecord {
    uint8_t bytes[ADDRESS_BYTES];
};

uint8_t feeAddressBytes[20] = {
    0x42, 0x52, 0x44, //BRD
    0x5F, 0x5F, // __
    'f', 'e', 'e', // fee
    0x5F, 0x5F, // __
    0x42, 0x52, 0x44, // BRD
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // padding
};

uint8_t unknownAddressBytes[20] = {
    0x42, 0x52, 0x44, //BRD
    0x5F, 0x5F, // __
    'u', 'n', 'k', 'n', 'o', 'w', 'n', // unknown
    0x5F, 0x5F, // __
    0x42, 0x52, 0x44, // BRD
    0x00, 0x00, 0x00 // padding
};

extern void rippleAddressFree (BRRippleAddress address)
{
    if (address) free(address);
}

BRRippleAddress rippleAddressCreateFeeAddress()
{
    BRRippleAddress address = calloc(1, sizeof(struct BRRippleAddressRecord));
    memcpy(address->bytes, feeAddressBytes, ADDRESS_BYTES);
    return address;
}

BRRippleAddress rippleAddressCreateUnknownAddress()
{
    BRRippleAddress address = calloc(1, sizeof(struct BRRippleAddressRecord));
    memcpy(address->bytes, unknownAddressBytes, ADDRESS_BYTES);
    return address;
}

extern int
rippleAddressIsFeeAddress (BRRippleAddress address)
{
    assert(address);
    if (memcmp(address->bytes, feeAddressBytes, sizeof(feeAddressBytes)) == 0) {
        return 1;
    } else {
        return 0;
    }
}

extern int
rippleAddressIsUnknownAddress (BRRippleAddress address)
{
    assert(address);
    if (memcmp(address->bytes, unknownAddressBytes, sizeof(unknownAddressBytes)) == 0) {
        return 1;
    } else {
        return 0;
    }
}

extern char * rippleAddressAsString (BRRippleAddress address)
{
    assert(address);
    char *string = calloc (1, 36);

    // Check for our special case __fee__ address
    // See the note above with respect to the feeAddressBytes
    if (rippleAddressIsFeeAddress (address)) {
        strcpy(string, "__fee__");
    } else if (rippleAddressIsUnknownAddress (address)) {
        strcpy (string, "unknown");
    } else {
        // The process is this:
        // 1. Prepend the Ripple address indicator (0) to the 20 bytes
        // 2. Do a douple sha265 hash on the bytes
        // 3. Use the first 4 bytes of the hash as checksum and append to the bytes
        uint8_t input[25];
        input[0] = 0; // Ripple address type
        memcpy(&input[1], address->bytes, 20);
        uint8_t hash[32];
        BRSHA256_2(hash, input, 21);
        memcpy(&input[21], hash, 4);

        // Now base58 encode the result
        rippleEncodeBase58(string, 35, input, 25);
        // NOTE: once the following function is "approved" we can switch to using it
        // and remove the base58 function above
        // static const char rippleAlphabet[] = "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";
        // BRBase58EncodeEx(string, 36, input, 25, rippleAlphabet);
    }
    return string;
}

extern BRRippleAddress // caller must free memory with rippleAddressFree
rippleAddressCreateFromKey (BRKey *publicKey)
{
    BRRippleAddress address = calloc(1, sizeof(struct BRRippleAddressRecord));
    UInt160 hash = BRKeyHash160(publicKey);
    memcpy(address->bytes, hash.u8, 20);
    return address;
}

extern BRRippleAddress // caller must free memory with rippleAddressFree
rippleAddressCreateFromBytes (uint8_t * buffer, int bufferSize)
{
    assert(buffer);
    assert(bufferSize == ADDRESS_BYTES);
    BRRippleAddress address = calloc(1, sizeof(struct BRRippleAddressRecord));
    memcpy(address->bytes, buffer, bufferSize);
    return address;
}

BRRippleAddress rippleAddressStringToAddress(const char* input)
{
    // The ripple address "string" is a base58 encoded string
    // using the ripple base58 alphabet.  After decoding the address
    // the output lookes like this
    // [ 0, 20-bytes, 4-byte checksum ] for a total of 25 bytes
    // In the ripple alphabet "r" maps to 0 which is why the first byte is 0

    // Decode the string input
    uint8_t bytes[25];
    //int length = BRBase58DecodeEx(NULL, 0, input, rippleAlphabet);
    int length = rippleDecodeBase58(input, NULL);
    if (length > 25) {
        // Since ripple addresses are created from 20 byte account IDs the
        // needed space to covert it back has to be 25 bytes.
        // LOG message?
        return 0;
    }

    // We got the correct length so go ahead and decode.
    rippleDecodeBase58(input, bytes);
    //BRBase58DecodeEx(bytes, 25, input, rippleAlphabet);

    // We need to do a checksum on all but the last 4 bytes.
    // From trial and error is appears that the checksum is just
    // sha256(sha256(first21bytes))
    uint8_t md32[32];
    BRSHA256_2(md32, &bytes[0], 21);
    // Compare the first 4 bytes of the sha hash to the last 4 bytes
    // of the decoded bytes (i.e. starting a byte 21)
    if (0 == memcmp(md32, &bytes[21], 4)) {
        // The checksum has passed so copy the 20 bytes that form the account id
        // to our ripple address structure.
        // i.e. strip off the 1 bytes token and the 4 byte checksum
        return rippleAddressCreateFromBytes(&bytes[1], 20);
    } else {
        // Checksum does not match - do we log this somewhere
        return NULL;
    }
    return NULL;
}

extern BRRippleAddress
rippleAddressCreateFromString(const char * rippleAddressString)
{
    // 2 special case so far - the __fee__ address and "unknown" address
    // See the note in BRRippleAcount.h with respect to the feeAddressBytes
    if (rippleAddressString == NULL || strlen(rippleAddressString) == 0 || strcmp(rippleAddressString, "unknown") == 0) {
        return rippleAddressCreateUnknownAddress ();
    } else if (strcmp(rippleAddressString, "__fee__") == 0) {
        return rippleAddressCreateFeeAddress ();
    } else {
        // Work backwards from this ripple address (string) to what is
        // known as the acount ID (20 bytes)
        return rippleAddressStringToAddress (rippleAddressString);
    }
}

extern int // 1 if equal
rippleAddressEqual (BRRippleAddress a1, BRRippleAddress a2) {
    return 0 == memcmp (a1->bytes, a2->bytes, 20);
}

extern int
rippleAddressGetRawSize (BRRippleAddress address)
{
    return ADDRESS_BYTES;
}

extern void rippleAddressGetRawBytes (BRRippleAddress address, uint8_t *buffer, int bufferSize)
{
    assert(buffer);
    assert(bufferSize >= ADDRESS_BYTES);
    memcpy(buffer, address->bytes, ADDRESS_BYTES);
}

extern BRRippleAddress rippleAddressClone (BRRippleAddress address)
{
    if (address) {
        BRRippleAddress clone = calloc(1, sizeof(struct BRRippleAddressRecord));
        *clone = *address;
        return clone;
    }
    return NULL;
}
