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
#include "BRBase58.h"
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
int rippleDecodeBase58(const char* input, uint8_t *output)
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

// NOTE: this is a copy from the one found in support
// TODO - modify the one in support to allow for different alphabets
size_t rippleEncodeBase58(char *str, size_t strLen, const uint8_t *data, size_t dataLen)
{
    static const char chars[] = "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";
    size_t i, j, len, zcount = 0;
    
    assert(data != NULL);
    while (zcount < dataLen && data && data[zcount] == 0) zcount++; // count leading zeroes
    
    uint8_t buf[(dataLen - zcount)*138/100 + 1]; // log(256)/log(58), rounded up
    
    memset(buf, 0, sizeof(buf));
    
    for (i = zcount; data && i < dataLen; i++) {
        uint32_t carry = data[i];
        
        for (j = sizeof(buf); j > 0; j--) {
            carry += (uint32_t)buf[j - 1] << 8;
            buf[j - 1] = carry % 58;
            carry /= 58;
        }
        
        var_clean(&carry);
    }
    
    i = 0;
    while (i < sizeof(buf) && buf[i] == 0) i++; // skip leading zeroes
    len = (zcount + sizeof(buf) - i) + 1;
    
    if (str && len <= strLen) {
        while (zcount-- > 0) *(str++) = chars[0];
        while (i < sizeof(buf)) *(str++) = chars[buf[i++]];
        *str = '\0';
    }
    
    mem_clean(buf, sizeof(buf));
    return (! str || len <= strLen) ? len : 0;
}
