//
//  BRBase58.h
//  breadwallet-core
//
//  Created by Aaron Voisine on 9/15/15.
//  Copyright (c) 2015 breadwallet LLC
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

#ifndef BRBase58_h
#define BRBase58_h

#include <stddef.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// base58 and base58check encoding: https://en.bitcoin.it/wiki/Base58Check_encoding

// returns the number of characters written to str including NULL terminator, or total strLen needed if str is NULL
// NOTE: this function calls BRBase58EncodeEx using the bitcoin alphabet. If you need to
// use a different alphabet call BRBase58EncodeEx directly
// NOTE: there is no equivalent for decode (currently) since the code is too different to
//       safely refactor
size_t BRBase58Encode(char *str, size_t strLen, const uint8_t *data, size_t dataLen);

// returns the number of bytes written to data, or total dataLen needed if data is NULL
size_t BRBase58Decode(uint8_t *data, size_t dataLen, const char *str);

// returns the number of characters written to str including NULL terminator, or total strLen needed if str is NULL
size_t BRBase58CheckEncode(char *str, size_t strLen, const uint8_t *data, size_t dataLen);

// returns the number of bytes written to data, or total dataLen needed if data is NULL
size_t BRBase58CheckDecode(uint8_t *data, size_t dataLen, const char *str);

// Extended versions of base58 encode/decode that allow caller to control
// the alphabet being used.  This is needed for Ripple (and perhaps others)

/*
 * Base58 encoder - with user supplied alphabet
 *
 * @param str      output buffer to hold encoded string
 * @param strLen   length of str buffer
 * @param data     input buffer - bytes to encode
 * @param dataLen  length of data buffer
 * @param alphabet the alphabet to use for base58 encoding
 *
 * @return number of characters written to str including NULL terminator
 *         or total strLen needed if str is NULL
 */
 size_t BRBase58EncodeEx(char *str, size_t strLen, const uint8_t *data, size_t dataLen, const char *alphabet);

/*
 * Base58 decoder - with user supplied alphabet
 *
 * @param  data      output buffer where decoded bytes will be copied
 * @param  dataLen   length of data buffer
 * @param  str       input string of base58 character to decode
 * @param  alphabet  the alphabet to use for base58 encoding
 *
 * @return the number of bytes written to data, or total dataLen needed if data is NULL
 */
size_t BRBase58DecodeEx(uint8_t *data, size_t dataLen, const char *str, const char *alphabet);

#ifdef __cplusplus
}
#endif

#endif // BRBase58_h
