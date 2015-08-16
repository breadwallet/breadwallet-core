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
#define BRBE16(x) (x)
#define BRLE16(x) ((((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8))
#define BRBE32(x) (x)
#define BRLE32(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
#define BRBE64(x) (x)
#define BRLE64(x) ((((x) & 0x00000000000000ffULL) << 56) | (((x) & 0xff00000000000000ULL) >> 56) |\
                   (((x) & 0x000000000000ff00ULL) << 40) | (((x) & 0x00ff000000000000ULL) >> 40) |\
                   (((x) & 0x0000000000ff0000ULL) << 24) | (((x) & 0x0000ff0000000000ULL) >> 24) |\
                   (((x) & 0x00000000ff000000ULL) << 8)  | (((x) & 0x000000ff00000000ULL) >> 8))
#else
#define BRBE16(x) ((((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8))
#define BRLE16(x) (x)
#define BRBE32(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
#define BRLE32(x) (x)
#define BRBE64(x) ((((x) & 0x00000000000000ffULL) << 56) | (((x) & 0xff00000000000000ULL) >> 56) |\
                   (((x) & 0x000000000000ff00ULL) << 40) | (((x) & 0x00ff000000000000ULL) >> 40) |\
                   (((x) & 0x0000000000ff0000ULL) << 24) | (((x) & 0x0000ff0000000000ULL) >> 24) |\
                   (((x) & 0x00000000ff000000ULL) << 8)  | (((x) & 0x000000ff00000000ULL) >> 8))
#define BRLE64(x) (x)
#endif

typedef union _BRUInt512 {
    unsigned char u8[512/8];
    unsigned short u16[512/16];
    unsigned int u32[512/32];
    unsigned long long u64[512/64];
} BRUInt512;

typedef union _BRUInt256 {
    unsigned char u8[256/8];
    unsigned short u16[256/16];
    unsigned int u32[256/32];
    unsigned long long u64[256/64];
} BRUInt256;

typedef union _BRUInt160 {
    unsigned char u8[160/8];
    unsigned short u16[160/16];
    unsigned int u32[160/32];
} BRUInt160;

typedef union _BRUInt128 {
    unsigned char u8[128/8];
    unsigned short u16[128/16];
    unsigned int u32[128/32];
    unsigned long long u64[128/64];
} BRUInt128;

#define br_uint512_eq(a, b)\
    ((a).u64[0] == (b).u64[0] && (a).u64[1] == (b).u64[1] && (a).u64[2] == (b).u64[2] && (a).u64[3] == (b).u64[3] &&\
    (a).u64[4] == (b).u64[4] && (a).u64[5] == (b).u64[5] && (a).u64[6] == (b).u64[6] && (a).u64[7] == (b).u64[7])

#define br_uint256_eq(a, b)\
    ((a).u64[0] == (b).u64[0] && (a).u64[1] == (b).u64[1] && (a).u64[2] == (b).u64[2] && (a).u64[3] == (b).u64[3])

#define br_uint160_eq(a, b)\
    ((a).u32[0] == (b).u32[0] && (a).u32[1] == (b).u32[1] && (a).u32[2] == (b).u32[2] && (a).u32[3] == (b).u32[3] &&\
    (a).u32[4] == (b).u32[4])

#define br_uint128_eq(a, b) ((a).u64[0] == (b).u64[0] && (a).u64[1] == (b).u64[1])

#define br_uint512_is_zero(u)\
    (((u).u64[0] | (u).u64[1] | (u).u64[2] | (u).u64[3] | (u).u64[4] | (u).u64[5] | (u).u64[6] | (u).u64[7]) == 0)
#define br_uint256_is_zero(u) (((u).u64[0] | (u).u64[1] | (u).u64[2] | (u).u64[3]) == 0)
#define br_uint160_is_zero(u) (((u).u32[0] | (u).u32[1] | (u).u32[2] | (u).u32[3] | (u).u32[4]) == 0)
#define br_uint128_is_zero(u) (((u).u64[0] | (u).u64[1]) == 0)

#define br_uint256_reverse(u) ((BRUInt256) { .u8 = {\
    (u).u8[31], (u).u8[30], (u).u8[29], (u).u8[28], (u).u8[27], (u).u8[26], (u).u8[25], (u).u8[24], (u).u8[23],\
    (u).u8[22], (u).u8[21], (u).u8[20], (u).u8[19], (u).u8[18], (u).u8[17], (u).u8[16], (u).u8[15], (u).u8[14],\
    (u).u8[13], (u).u8[12], (u).u8[11], (u).u8[10], (u).u8[9], (u).u8[8], (u).u8[7], (u).u8[6], (u).u8[5], (u).u8[4],\
    (u).u8[3], (u).u8[2], (u).u8[1], (u).u8[0] } })

#define BRUINT512_ZERO ((BRUInt512) { .u64 = { 0, 0, 0, 0, 0, 0, 0, 0 } })
#define BRUINT256_ZERO ((BRUInt256) { .u64 = { 0, 0, 0, 0 } })
#define BRUINT160_ZERO ((BRUInt160) { .u32 = { 0, 0, 0, 0, 0 } })
#define BRUINT128_ZERO ((BRUInt128) { .u64 = { 0, 0 } })

void BRSHA1(const void *data, size_t len, unsigned char *md);

void BRSHA256(const void *data, size_t len, unsigned char *md);

void BRSHA256_2(const void *data, size_t len, unsigned char *md);

void BRSHA512(const void *data, size_t len, unsigned char *md);

// ripemd-160 hash function: http://homes.esat.kuleuven.be/~bosselae/ripemd160.html
void BRRMD160(const void *data, size_t len, unsigned char *md);

void BRHash160(const void *data, size_t len, unsigned char *md);

void BRHMAC(void (*hash)(const void *, size_t, unsigned char *), size_t mdlen, const void *key, size_t klen,
            const void *data, size_t dlen, unsigned char *md);

#endif // BRHash_h
