//
//  BRInt.h
//
//  Created by Aaron Voisine on 8/16/15.
//  Copyright (c) 2015 breadwallet LLC.
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

#ifndef BRInt_h
#define BRInt_h

#include <stdint.h>

// large integers

typedef union {
    uint8_t u8[128/8];
    uint16_t u16[128/16];
    uint32_t u32[128/32];
    uint64_t u64[128/64];
} UInt128;

typedef union {
    uint8_t u8[160/8];
    uint16_t u16[160/16];
    uint32_t u32[160/32];
} UInt160;

typedef union {
    uint8_t u8[256/8];
    uint16_t u16[256/16];
    uint32_t u32[256/32];
    uint64_t u64[256/64];
} UInt256;

typedef union {
    uint8_t u8[512/8];
    uint16_t u16[512/16];
    uint32_t u32[512/32];
    uint64_t u64[512/64];
} UInt512;

inline static int UInt128Eq(UInt128 a, UInt128 b)
{
    return (a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1]);
}

inline static int UInt160Eq(UInt160 a, UInt160 b)
{
    return (a.u32[0] == b.u32[0] && a.u32[1] == b.u32[1] && a.u32[2] == b.u32[2] && a.u32[3] == b.u32[3] &&
            a.u32[4] == b.u32[4]);
}

inline static int UInt256Eq(UInt256 a, UInt256 b)
{
    return (a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1] && a.u64[2] == b.u64[2] && a.u64[3] == b.u64[3]);
}

inline static int UInt512Eq(UInt512 a, UInt512 b)
{
    return (a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1] && a.u64[2] == b.u64[2] && a.u64[3] == b.u64[3] &&
            a.u64[4] == b.u64[4] && a.u64[5] == b.u64[5] && a.u64[6] == b.u64[6] && a.u64[7] == b.u64[7]);
}

inline static int UInt128IsZero(UInt128 u)
{
    return ((u.u64[0] | u.u64[1]) == 0);
}

inline static int UInt160IsZero(UInt160 u)
{
    return ((u.u32[0] | u.u32[1] | u.u32[2] | u.u32[3] | u.u32[4]) == 0);
}

inline static int UInt256IsZero(UInt256 u)
{
    return ((u.u64[0] | u.u64[1] | u.u64[2] | u.u64[3]) == 0);
}

inline static int UInt512IsZero(UInt512 u)
{
    return ((u.u64[0] | u.u64[1] | u.u64[2] | u.u64[3] | u.u64[4] | u.u64[5] | u.u64[6] | u.u64[7]) == 0);
}

inline static UInt256 UInt256Reverse(UInt256 u)
{
    return ((UInt256) { .u8 = { u.u8[31], u.u8[30], u.u8[29], u.u8[28], u.u8[27], u.u8[26], u.u8[25], u.u8[24],
                                u.u8[23], u.u8[22], u.u8[21], u.u8[20], u.u8[19], u.u8[18], u.u8[17], u.u8[16],
                                u.u8[15], u.u8[14], u.u8[13], u.u8[12], u.u8[11], u.u8[10], u.u8[9],  u.u8[8],
                                u.u8[7],  u.u8[6],  u.u8[5],  u.u8[4],  u.u8[3],  u.u8[2],  u.u8[1],  u.u8[0] } });
}

#define UINT128_ZERO ((UInt128) { .u64 = { 0, 0 } })
#define UINT160_ZERO ((UInt160) { .u32 = { 0, 0, 0, 0, 0 } })
#define UINT256_ZERO ((UInt256) { .u64 = { 0, 0, 0, 0 } })
#define UINT512_ZERO ((UInt512) { .u64 = { 0, 0, 0, 0, 0, 0, 0, 0 } })

#define uint256_hex_str(u) ((const char[]) {\
    _hex_char((u).u8[ 0] >> 4), _hex_char((u).u8[ 0] & 0xf), _hex_char((u).u8[ 1] >> 4), _hex_char((u).u8[ 1] & 0xf),\
    _hex_char((u).u8[ 2] >> 4), _hex_char((u).u8[ 2] & 0xf), _hex_char((u).u8[ 3] >> 4), _hex_char((u).u8[ 3] & 0xf),\
    _hex_char((u).u8[ 4] >> 4), _hex_char((u).u8[ 4] & 0xf), _hex_char((u).u8[ 5] >> 4), _hex_char((u).u8[ 5] & 0xf),\
    _hex_char((u).u8[ 6] >> 4), _hex_char((u).u8[ 6] & 0xf), _hex_char((u).u8[ 7] >> 4), _hex_char((u).u8[ 7] & 0xf),\
    _hex_char((u).u8[ 8] >> 4), _hex_char((u).u8[ 8] & 0xf), _hex_char((u).u8[ 9] >> 4), _hex_char((u).u8[ 9] & 0xf),\
    _hex_char((u).u8[10] >> 4), _hex_char((u).u8[10] & 0xf), _hex_char((u).u8[11] >> 4), _hex_char((u).u8[11] & 0xf),\
    _hex_char((u).u8[12] >> 4), _hex_char((u).u8[12] & 0xf), _hex_char((u).u8[13] >> 4), _hex_char((u).u8[13] & 0xf),\
    _hex_char((u).u8[14] >> 4), _hex_char((u).u8[14] & 0xf), _hex_char((u).u8[15] >> 4), _hex_char((u).u8[15] & 0xf),\
    _hex_char((u).u8[16] >> 4), _hex_char((u).u8[16] & 0xf), _hex_char((u).u8[17] >> 4), _hex_char((u).u8[17] & 0xf),\
    _hex_char((u).u8[18] >> 4), _hex_char((u).u8[18] & 0xf), _hex_char((u).u8[19] >> 4), _hex_char((u).u8[19] & 0xf),\
    _hex_char((u).u8[20] >> 4), _hex_char((u).u8[20] & 0xf), _hex_char((u).u8[21] >> 4), _hex_char((u).u8[21] & 0xf),\
    _hex_char((u).u8[22] >> 4), _hex_char((u).u8[22] & 0xf), _hex_char((u).u8[23] >> 4), _hex_char((u).u8[23] & 0xf),\
    _hex_char((u).u8[24] >> 4), _hex_char((u).u8[24] & 0xf), _hex_char((u).u8[25] >> 4), _hex_char((u).u8[25] & 0xf),\
    _hex_char((u).u8[26] >> 4), _hex_char((u).u8[26] & 0xf), _hex_char((u).u8[27] >> 4), _hex_char((u).u8[27] & 0xf),\
    _hex_char((u).u8[28] >> 4), _hex_char((u).u8[28] & 0xf), _hex_char((u).u8[29] >> 4), _hex_char((u).u8[29] & 0xf),\
    _hex_char((u).u8[30] >> 4), _hex_char((u).u8[30] & 0xf), _hex_char((u).u8[31] >> 4), _hex_char((u).u8[31] & 0xf),\
    '\0' })

#define uint256_hex_decode(s) ((UInt256) { .u8 = {\
    (_hex_val((s)[ 0]) << 4) | _hex_val((s)[ 1]), (_hex_val((s)[ 2]) << 4) | _hex_val((s)[ 3]),\
    (_hex_val((s)[ 4]) << 4) | _hex_val((s)[ 5]), (_hex_val((s)[ 6]) << 4) | _hex_val((s)[ 7]),\
    (_hex_val((s)[ 8]) << 4) | _hex_val((s)[ 9]), (_hex_val((s)[10]) << 4) | _hex_val((s)[11]),\
    (_hex_val((s)[12]) << 4) | _hex_val((s)[13]), (_hex_val((s)[14]) << 4) | _hex_val((s)[15]),\
    (_hex_val((s)[16]) << 4) | _hex_val((s)[17]), (_hex_val((s)[18]) << 4) | _hex_val((s)[19]),\
    (_hex_val((s)[20]) << 4) | _hex_val((s)[21]), (_hex_val((s)[22]) << 4) | _hex_val((s)[23]),\
    (_hex_val((s)[24]) << 4) | _hex_val((s)[25]), (_hex_val((s)[26]) << 4) | _hex_val((s)[27]),\
    (_hex_val((s)[28]) << 4) | _hex_val((s)[29]), (_hex_val((s)[30]) << 4) | _hex_val((s)[31]),\
    (_hex_val((s)[32]) << 4) | _hex_val((s)[33]), (_hex_val((s)[34]) << 4) | _hex_val((s)[35]),\
    (_hex_val((s)[36]) << 4) | _hex_val((s)[37]), (_hex_val((s)[38]) << 4) | _hex_val((s)[39]),\
    (_hex_val((s)[40]) << 4) | _hex_val((s)[41]), (_hex_val((s)[42]) << 4) | _hex_val((s)[43]),\
    (_hex_val((s)[44]) << 4) | _hex_val((s)[45]), (_hex_val((s)[46]) << 4) | _hex_val((s)[47]),\
    (_hex_val((s)[48]) << 4) | _hex_val((s)[49]), (_hex_val((s)[50]) << 4) | _hex_val((s)[51]),\
    (_hex_val((s)[52]) << 4) | _hex_val((s)[53]), (_hex_val((s)[54]) << 4) | _hex_val((s)[55]),\
    (_hex_val((s)[56]) << 4) | _hex_val((s)[57]), (_hex_val((s)[58]) << 4) | _hex_val((s)[59]),\
    (_hex_val((s)[60]) << 4) | _hex_val((s)[61]), (_hex_val((s)[62]) << 4) | _hex_val((s)[63]) } })

#define _hex_val(c) (((c) >= '0' && (c) <= '9') ? (c) - '0' :\
                     ((c) >= 'A' && (c) <= 'F') ? (c) - 'A' :\
                     ((c) >= 'a' && (c) <= 'f') ? (c) - 'a' : -1)

#define _hex_char(u) ((u) + (((u) <= 9) ? '0' : (((u) <= 0xf) ? 'a' - 10 : '?')))

// integer endian swapping (detects endianess with predefined macros in clang and gcc, and msvc is always little endian)

#if __BIG_ENDIAN__ || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

#define WORDS_BIGENDIAN 1
#define be16(x) (x)
#define le16(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define be32(x) (x)
#define le32(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
#define be64(x) (x)
#define le64(x) ((((x) & 0x00000000000000ffULL) << 56) | (((x) & 0xff00000000000000ULL) >> 56) |\
                 (((x) & 0x000000000000ff00ULL) << 40) | (((x) & 0x00ff000000000000ULL) >> 40) |\
                 (((x) & 0x0000000000ff0000ULL) << 24) | (((x) & 0x0000ff0000000000ULL) >> 24) |\
                 (((x) & 0x00000000ff000000ULL) << 8)  | (((x) & 0x000000ff00000000ULL) >> 8))

#else

#define be16(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define le16(x) (x)
#define be32(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
#define le32(x) (x)
#define be64(x) ((((x) & 0x00000000000000ffULL) << 56) | (((x) & 0xff00000000000000ULL) >> 56) |\
                 (((x) & 0x000000000000ff00ULL) << 40) | (((x) & 0x00ff000000000000ULL) >> 40) |\
                 (((x) & 0x0000000000ff0000ULL) << 24) | (((x) & 0x0000ff0000000000ULL) >> 24) |\
                 (((x) & 0x00000000ff000000ULL) << 8)  | (((x) & 0x000000ff00000000ULL) >> 8))
#define le64(x) (x)

#endif

#endif // BRInt_h
