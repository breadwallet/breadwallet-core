//
//  BRKey.h
//
//  Created by Aaron Voisine on 8/19/15.
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

#ifndef BRKey_h
#define BRKey_h

#include <stddef.h>
#include "BRTypes.h"

typedef struct {
    uint8_t u8[33];
} BRPubKey;

#define BR_PUBKEY_NONE ((BRPubKey)\
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })

UInt256 secp256k1_mod_add(UInt256 a, UInt256 b); // add 256bit big endian ints (mod secp256k1 order)
UInt256 secp256k1_mod_mul(UInt256 a, UInt256 b); // multiply 256bit big endian ints (mod secp256k1 order)
int secp256k1_point_add(void *r, const void *a, const void *b, int compressed); // add secp256k1 ec-points
int secp256k1_point_mul(void *r, const void *p, UInt256 i, int compressed);// multiply ec-point by 256bit big endian int

size_t BRKeyPrivKey(char *privKey, size_t len, UInt256 secret, int compressed);

size_t BRKeyPubKey(void *pubKey, size_t len, const char *privKey);

size_t BRKeyAddress(char *address, size_t addrLen, const void *pubKey);

UInt160 BRKeyHash160(const void *pubKey);

size_t BRKeySign(void *sig, size_t len, const char *privKey, UInt256 md);

int BRKeyVerify(const void *pubKey, const void *sig, size_t len);

#endif // BRKey_h
