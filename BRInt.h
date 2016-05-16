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

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STACK 0x20000 // maximum buffer size to allocate on the stack, 128k

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
                                u.u8[15], u.u8[14], u.u8[13], u.u8[12], u.u8[11], u.u8[10], u.u8[ 9], u.u8[ 8],
                                u.u8[ 7], u.u8[ 6], u.u8[5],  u.u8[ 4], u.u8[ 3], u.u8[ 2], u.u8[ 1], u.u8[ 0] } });
}

#define UINT128_ZERO ((UInt128) { .u64 = { 0, 0 } })
#define UINT160_ZERO ((UInt160) { .u32 = { 0, 0, 0, 0, 0 } })
#define UINT256_ZERO ((UInt256) { .u64 = { 0, 0, 0, 0 } })
#define UINT512_ZERO ((UInt512) { .u64 = { 0, 0, 0, 0, 0, 0, 0, 0 } })

#define uint256_hex_encode(u) ((const char[]) {\
    _hexc((u).u8[ 0] >> 4), _hexc((u).u8[ 0]), _hexc((u).u8[ 1] >> 4), _hexc((u).u8[ 1]),\
    _hexc((u).u8[ 2] >> 4), _hexc((u).u8[ 2]), _hexc((u).u8[ 3] >> 4), _hexc((u).u8[ 3]),\
    _hexc((u).u8[ 4] >> 4), _hexc((u).u8[ 4]), _hexc((u).u8[ 5] >> 4), _hexc((u).u8[ 5]),\
    _hexc((u).u8[ 6] >> 4), _hexc((u).u8[ 6]), _hexc((u).u8[ 7] >> 4), _hexc((u).u8[ 7]),\
    _hexc((u).u8[ 8] >> 4), _hexc((u).u8[ 8]), _hexc((u).u8[ 9] >> 4), _hexc((u).u8[ 9]),\
    _hexc((u).u8[10] >> 4), _hexc((u).u8[10]), _hexc((u).u8[11] >> 4), _hexc((u).u8[11]),\
    _hexc((u).u8[12] >> 4), _hexc((u).u8[12]), _hexc((u).u8[13] >> 4), _hexc((u).u8[13]),\
    _hexc((u).u8[14] >> 4), _hexc((u).u8[14]), _hexc((u).u8[15] >> 4), _hexc((u).u8[15]),\
    _hexc((u).u8[16] >> 4), _hexc((u).u8[16]), _hexc((u).u8[17] >> 4), _hexc((u).u8[17]),\
    _hexc((u).u8[18] >> 4), _hexc((u).u8[18]), _hexc((u).u8[19] >> 4), _hexc((u).u8[19]),\
    _hexc((u).u8[20] >> 4), _hexc((u).u8[20]), _hexc((u).u8[21] >> 4), _hexc((u).u8[21]),\
    _hexc((u).u8[22] >> 4), _hexc((u).u8[22]), _hexc((u).u8[23] >> 4), _hexc((u).u8[23]),\
    _hexc((u).u8[24] >> 4), _hexc((u).u8[24]), _hexc((u).u8[25] >> 4), _hexc((u).u8[25]),\
    _hexc((u).u8[26] >> 4), _hexc((u).u8[26]), _hexc((u).u8[27] >> 4), _hexc((u).u8[27]),\
    _hexc((u).u8[28] >> 4), _hexc((u).u8[28]), _hexc((u).u8[29] >> 4), _hexc((u).u8[29]),\
    _hexc((u).u8[30] >> 4), _hexc((u).u8[30]), _hexc((u).u8[31] >> 4), _hexc((u).u8[31]), '\0' })

#define uint256_hex_decode(s) ((UInt256) { .u8 = {\
    (_hexu((s)[ 0]) << 4) | _hexu((s)[ 1]), (_hexu((s)[ 2]) << 4) | _hexu((s)[ 3]),\
    (_hexu((s)[ 4]) << 4) | _hexu((s)[ 5]), (_hexu((s)[ 6]) << 4) | _hexu((s)[ 7]),\
    (_hexu((s)[ 8]) << 4) | _hexu((s)[ 9]), (_hexu((s)[10]) << 4) | _hexu((s)[11]),\
    (_hexu((s)[12]) << 4) | _hexu((s)[13]), (_hexu((s)[14]) << 4) | _hexu((s)[15]),\
    (_hexu((s)[16]) << 4) | _hexu((s)[17]), (_hexu((s)[18]) << 4) | _hexu((s)[19]),\
    (_hexu((s)[20]) << 4) | _hexu((s)[21]), (_hexu((s)[22]) << 4) | _hexu((s)[23]),\
    (_hexu((s)[24]) << 4) | _hexu((s)[25]), (_hexu((s)[26]) << 4) | _hexu((s)[27]),\
    (_hexu((s)[28]) << 4) | _hexu((s)[29]), (_hexu((s)[30]) << 4) | _hexu((s)[31]),\
    (_hexu((s)[32]) << 4) | _hexu((s)[33]), (_hexu((s)[34]) << 4) | _hexu((s)[35]),\
    (_hexu((s)[36]) << 4) | _hexu((s)[37]), (_hexu((s)[38]) << 4) | _hexu((s)[39]),\
    (_hexu((s)[40]) << 4) | _hexu((s)[41]), (_hexu((s)[42]) << 4) | _hexu((s)[43]),\
    (_hexu((s)[44]) << 4) | _hexu((s)[45]), (_hexu((s)[46]) << 4) | _hexu((s)[47]),\
    (_hexu((s)[48]) << 4) | _hexu((s)[49]), (_hexu((s)[50]) << 4) | _hexu((s)[51]),\
    (_hexu((s)[52]) << 4) | _hexu((s)[53]), (_hexu((s)[54]) << 4) | _hexu((s)[55]),\
    (_hexu((s)[56]) << 4) | _hexu((s)[57]), (_hexu((s)[58]) << 4) | _hexu((s)[59]),\
    (_hexu((s)[60]) << 4) | _hexu((s)[61]), (_hexu((s)[62]) << 4) | _hexu((s)[63]) } })

#define _hexc(u) (((u) & 0x0f) + ((((u) & 0x0f) <= 9) ? '0' : 'a' - 0x0a))
#define _hexu(c) (((c) >= '0' && (c) <= '9') ? (c) - '0' : ((c) >= 'a' && (c) <= 'f') ? (c) - ('a' - 0x0a) :\
                  ((c) >= 'A' && (c) <= 'F') ? (c) - ('A' - 0x0a) : 0)

// unaligned memory access

union _u16 { uint8_t u8[16/8]; uint16_t u16; };
union _u32 { uint8_t u8[32/8]; uint32_t u32; };
union _u64 { uint8_t u8[64/8]; uint64_t u64; };
    
#define set16be(r, x) (*(union _u16 *)(r) = (union _u16) { ((x) >> 8) & 0xff, (x) & 0xff })
#define set16le(r, x) (*(union _u16 *)(r) = (union _u16) { (x) & 0xff, ((x) >> 8) & 0xff })
#define set32be(r, x) (*(union _u32 *)(r) =\
    (union _u32) { ((x) >> 24) & 0xff, ((x) >> 16) & 0xff, ((x) >> 8) & 0xff, (x) & 0xff })
#define set32le(r, x) (*(union _u32 *)(r) =\
    (union _u32) { (x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, ((x) >> 24) & 0xff })
#define set64be(r, x) (*(union _u64 *)(r) =\
    (union _u64) { ((x) >> 56) & 0xff, ((x) >> 48) & 0xff, ((x) >> 40) & 0xff, ((x) >> 32) & 0xff,\
                   ((x) >> 24) & 0xff, ((x) >> 16) & 0xff, ((x) >> 8) & 0xff, (x) & 0xff })
#define set64le(r, x) (*(union _u64 *)(r) =\
    (union _u64) { (x) & 0xff, ((x) >> 8) & 0xff, ((x) >> 16) & 0xff, ((x) >> 24) & 0xff,\
                   ((x) >> 32) & 0xff, ((x) >> 40) & 0xff, ((x) >> 48) & 0xff, ((x) >> 56) & 0xff })

#define get16be(x) (((uint16_t)((uint8_t *)(x))[0] << 8)  | ((uint16_t)((uint8_t *)(x))[1]))
#define get16le(x) (((uint16_t)((uint8_t *)(x))[1] << 8)  | ((uint16_t)((uint8_t *)(x))[0]))
#define get32be(x) (((uint32_t)((uint8_t *)(x))[0] << 24) | ((uint32_t)((uint8_t *)(x))[1] << 16) |\
                    ((uint32_t)((uint8_t *)(x))[2] << 8)  | ((uint32_t)((uint8_t *)(x))[3]))
#define get32le(x) (((uint32_t)((uint8_t *)(x))[3] << 24) | ((uint32_t)((uint8_t *)(x))[2] << 16) |\
                    ((uint32_t)((uint8_t *)(x))[1] << 8)  | ((uint32_t)((uint8_t *)(x))[0]))
#define get64be(x) (((uint64_t)((uint8_t *)(x))[0] << 56) | ((uint64_t)((uint8_t *)(x))[1] << 48) |\
                    ((uint64_t)((uint8_t *)(x))[2] << 40) | ((uint64_t)((uint8_t *)(x))[3] << 32) |\
                    ((uint64_t)((uint8_t *)(x))[4] << 24) | ((uint64_t)((uint8_t *)(x))[5] << 16) |\
                    ((uint64_t)((uint8_t *)(x))[6] << 8)  | ((uint64_t)((uint8_t *)(x))[7]))
#define get64le(x) (((uint64_t)((uint8_t *)(x))[7] << 56) | ((uint64_t)((uint8_t *)(x))[6] << 48) |\
                    ((uint64_t)((uint8_t *)(x))[5] << 40) | ((uint64_t)((uint8_t *)(x))[4] << 32) |\
                    ((uint64_t)((uint8_t *)(x))[3] << 24) | ((uint64_t)((uint8_t *)(x))[2] << 16) |\
                    ((uint64_t)((uint8_t *)(x))[1] << 8)  | ((uint64_t)((uint8_t *)(x))[0]))

#ifdef __cplusplus
}
#endif

#endif // BRInt_h
