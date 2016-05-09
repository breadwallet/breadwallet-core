//
//  BRHash.h
//
//  Created by Aaron Voisine on 8/8/15.
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

#ifndef BRHash_h
#define BRHash_h

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// sha-1 - not recommended for cryptographic use
void BRSHA1(void *md20, const void *data, size_t len);

void BRSHA256(void *md32, const void *data, size_t len);

void BRSHA224(void *md28, const void *data, size_t len);

// double-sha-256 = sha-256(sha-256(x))
void BRSHA256_2(void *md32, const void *data, size_t len);

void BRSHA384(void *md48, const void *data, size_t len);

void BRSHA512(void *md64, const void *data, size_t len);

// ripemd-160: http://homes.esat.kuleuven.be/~bosselae/ripemd160.html
void BRRMD160(void *md20, const void *data, size_t len);

// bitcoin hash-160 = ripemd-160(sha-256(x))
void BRHash160(void *md20, const void *data, size_t len);

// md5 - for non-cryptographic use only
void BRMD5(void *md16, const void *data, size_t len);

void BRHMAC(void *md, void (*hash)(void *, const void *, size_t), size_t hashLen, const void *key, size_t keyLen,
            const void *data, size_t dataLen);

void BRPBKDF2(void *dk, size_t dkLen, void (*hash)(void *, const void *, size_t), size_t hashLen,
              const void *pw, size_t pwLen, const void *salt, size_t saltLen, unsigned rounds);

// murmurHash3 (x86_32): https://code.google.com/p/smhasher/ - for non cryptographic use only
uint32_t BRMurmur3_32(const void *data, size_t len, uint32_t seed);

#ifdef __cplusplus
}
#endif

#endif // BRHash_h
