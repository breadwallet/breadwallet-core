//
//  BRRippleBase58.c
//  Core
//
//  Created by Carl Cherry on May 3, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "BRRippleBase58.h"
#include "BRCrypto.h"


//------------------------------------------------------------------------------
/*
 This file is part of rippled: https://github.com/ripple/rippled
 Copyright (c) 2012, 2013 Ripple Labs Inc.
 
 Permission to use, copy, modify, and/or distribute this software for any
 purpose  with  or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.
 
 THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
 MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
 ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
//==============================================================================


static char rippleAlphabet[] = "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";

/**
 * The following 2 C fuctions are based on C++ code found in the following
 * file https://github.com/ripple/rippled/blob/develop/src/ripple/protocol/impl/tokens.cpp
 * with the following copyright notice
 * This file is part of rippled: https://github.com/ripple/rippled
 * Copyright (c) 2012, 2013 Ripple Labs Inc.
 */
int decodeBase58(const char* input, uint8_t *output)
{
    int inv[256];
    memset(&inv[0], -1, sizeof(inv));
    int map_num = 0;
    for (int i = 0; i < strlen(rippleAlphabet); i++) {
        inv[rippleAlphabet[i]] = map_num++;
    }
    
    const char *psz = input;
    int remain = (int)strlen(input);
    
    // Skip and count leading zeroes
    int zeroes = 0;
    while (remain > 0 && inv[*psz] == 0)
    {
        ++zeroes;
        ++psz;
        --remain;
    }
    
    // Allocate enough space in big-endian base256 representation.
    // log(58) / log(256), rounded up.
    int b256_size = remain * 733 / 1000 + 1;
    unsigned char b256[b256_size];
    memset(b256, 0x00, b256_size);

    // The caller might just be asking for the size
    if (!output) {
        return b256_size;
    }

    // Do the decoding
    while (remain > 0)
    {
        int carry = inv[*psz];
        if (carry == -1)
            return 0;
        // Apply "b256 = b256 * 58 + carry".
        for (int i = b256_size - 1; i >= 0; i--)
        {
            carry += 58 * b256[i];
            b256[i] = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        ++psz;
        --remain;
    }
    
    // Skip leading zeroes in the decoded bytes
    int b256_index = 0;
    while(b256[b256_index] == 0) { b256_index++; }
    
    // Copy the number of leading zeroes to the output
    int output_index = 0;
    for(int i = 0; i < zeroes; i++) {
        output[output_index++] = 0x00;
    }
    // Now copy the remainder of the decoded bytes to the output buffer
    for(int i = b256_index; i < b256_size; i++) {
        output[output_index++] = b256[i];
    }
    
    return b256_size;
}

extern int rippleAddressStringToAddress(const char* input, BRRippleAddress *address)
{
    // The ripple address "string" is a base58 encoded string
    // using the ripple base58 alphabet.  After decoding the address
    // the output lookes like this
    // [ 0, 20-bytes, 4-byte checksum ] for a total of 25 bytes
    // In the ripple alphabet "r" maps to 0 which is why the first byte is 0
    assert(address);

    // Decode the
    uint8_t bytes[25];
    int length = decodeBase58(input, NULL);
    if (length != 25) {
        // Since ripple addresses are created from 20 byte account IDs the
        // needed space to covert it back has to be 25 bytes.
        // LOG message?
        return 0;
    }

    // We got the correct length so go ahead and decode.
    decodeBase58(input, bytes);

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
        memcpy(address->bytes, &bytes[1], 20);
        return 20;
    } else {
        // Checksum does not match - do we log this somewhere
    }
    return 0;
}

