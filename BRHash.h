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
//

#ifndef BRHash_h
#define BRHash_h

#include <stddef.h>

#if !defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
#error endianess is unkown
#endif

#if __BIG_ENDIAN__
#define be16(x) (x)
#define le16(x) ((((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8))
#define be32(x) (x)
#define le32(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
#define be64(x) (x)
#define le64(x) ((((x) & 0x00000000000000ffULL) << 56) | (((x) & 0xff00000000000000ULL) >> 56) |\
                 (((x) & 0x000000000000ff00ULL) << 40) | (((x) & 0x00ff000000000000ULL) >> 40) |\
                 (((x) & 0x0000000000ff0000ULL) << 24) | (((x) & 0x0000ff0000000000ULL) >> 24) |\
                 (((x) & 0x00000000ff000000ULL) << 8)  | (((x) & 0x000000ff00000000ULL) >> 8))
#else
#define be16(x) ((((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8))
#define le16(x) (x)
#define be32(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
#define le32(x) (x)
#define be64(x) ((((x) & 0x00000000000000ffULL) << 56) | (((x) & 0xff00000000000000ULL) >> 56) |\
                 (((x) & 0x000000000000ff00ULL) << 40) | (((x) & 0x00ff000000000000ULL) >> 40) |\
                 (((x) & 0x0000000000ff0000ULL) << 24) | (((x) & 0x0000ff0000000000ULL) >> 24) |\
                 (((x) & 0x00000000ff000000ULL) << 8)  | (((x) & 0x000000ff00000000ULL) >> 8))
#define le64(x) (x)
#endif

void BRSHA1(const void *data, size_t len, void *md);

void BRSHA256(const void *data, size_t len, void *md);

void BRSHA256_2(const void *data, size_t len, void *md);

void BRSHA512(const void *data, size_t len, void *md);

// ripemd-160 hash function: http://homes.esat.kuleuven.be/~bosselae/ripemd160.html
void BRRMD160(const void *data, size_t len, void *md);

void BRHash160(const void *data, size_t len, void *md);

void BRHMAC(void (*hash)(const void *, size_t, void *), int hlen, const void *key, size_t klen,
            const void *data, size_t dlen, void *md);

void BRPBKDF2(void (*hash)(const void *, size_t, void *), int hlen, const void *pw, size_t pwlen,
              const void *salt, size_t slen, unsigned rounds, void *dk, size_t dklen);

// scrypt key derivation: http://www.tarsnap.com/scrypt.html
void BRScrypt(const void *pw, size_t pwlen, const void *salt, size_t slen, long n, int r, int p,
              void *dk, size_t dklen);

#endif // BRHash_h
