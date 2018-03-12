//
//  BBREthereumMath.c
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/10/2018.
//  Copyright (c) 2018 breadwallet LLC
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

#include "BREthereumMath.h"

#define AS_UINT64(x)  ((uint64_t) (x))

extern UInt256
createUInt256 (uint64_t value) {
  UInt256 result = { .u64 = { value, 0, 0, 0}};
  return result;
}

extern UInt256
addUInt256_Overflow (const UInt256 y, const UInt256 x, int *overflow) {
  assert (overflow != NULL);

  UInt256 z = UINT256_ZERO;
  unsigned int count = sizeof (UInt256) / sizeof(uint32_t);

  // x = xa*2^0 + xb*2^32 + ...
  // y = ya*2^0 + yb*2^32 + ...
  // z = (xa + ya)*2^0 + (xb + yb)*2^32 + ...
  uint64_t carry = 0;
  for (int i = 0; i < count; i++) {
    uint64_t sum = AS_UINT64(x.u32[i]) + AS_UINT64(y.u32[i]) + carry;
    carry = sum >> 32;
    z.u32[i] = (uint32_t) sum;
  }

  *overflow = (int) carry;
  return (0 != carry
          ? UINT256_ZERO
          : z);
}

extern UInt512
addUInt256 (UInt256 x, UInt256 y) {
  UInt512 z = UINT512_ZERO;
  unsigned int count = sizeof (UInt256) / sizeof(uint32_t);

  // x = xa*2^0 + xb*2^32 + ...
  // y = ya*2^0 + yb*2^32 + ...
  // z = (xa + ya)*2^0 + (xb + yb)*2^32 + ...
  uint64_t carry = 0;
  for (int i = 0; i < count; i++) {
    uint64_t sum = AS_UINT64(x.u32[i]) + AS_UINT64(y.u32[i]) + carry;
    carry = sum >> 32;
    z.u32[i] = (uint32_t) sum;
  }
  z.u32[count] = (uint32_t) carry;
  return z;
}

static UInt256
subUInt256_x_gt_y (UInt256 x, UInt256 y) {
  UInt256 z = UINT256_ZERO;
  unsigned int count = sizeof (UInt256) / sizeof(uint32_t);

  uint64_t borrow = 0;
  for (int i = 0; i < count; i++) {
    uint64_t diff;
    if (AS_UINT64(x.u32[i]) >= (AS_UINT64(y.u32[i]) + borrow)) {
      diff = AS_UINT64(x.u32[i]) - (AS_UINT64(y.u32[i]) + borrow);
      borrow = 0;
    }
    else {
      diff = ((AS_UINT64(1) << 32) + AS_UINT64(x.u32[i])) - (AS_UINT64(y.u32[i]) + borrow);
      borrow = 1;
    }
    z.u32[i] = (uint32_t) diff;
  }
  return z;
}

extern UInt256
subUInt256_Negative (UInt256 x, UInt256 y, int *negative) {
  assert (negative != NULL);
  if (eqUInt256(x, y)) {
    *negative = 0;
    return UINT256_ZERO;
  }
  else if (gtUInt256(x, y)) {
    *negative = 0;
    return subUInt256_x_gt_y(x, y);
  }
  else {
    *negative = 1;
    return subUInt256_x_gt_y(y, x);
  }
}


extern UInt512
mulUInt256 (const UInt256 x, const UInt256 y) {
  //  assert (__LITTLE_ENDIAN__ == BYTE_ORDER);
  UInt512 z = UINT512_ZERO;

  unsigned int count = sizeof (UInt256) / sizeof(uint32_t);

  // Use 'grade school' long multiplication in base 32.  For UInt256 we'll have 8 32-bit value
  // and perform 64 32-bit multiplications.  A more sophisticated algorith, e.g. Katasuba, can
  // perform just 27 32-bit multiplications.  For our application, not a big enough savings for
  // the added complexity.
  for (int xi = 0; xi < count; xi++) {
    uint64_t carry = 0;
    if (x.u32[xi] == 0) continue;
    for (int yi = 0; yi < count; yi++) {
      uint64_t total = z.u32[yi + xi] + carry + AS_UINT64 (y.u32[yi]) * AS_UINT64 (x.u32[xi]);
      carry = total >> 32;
      z.u32[yi + xi] = (uint32_t) total;
    }
    z.u32[xi + count] += carry;
  }
  return z;
}

extern UInt256
mulUInt256_Overflow (UInt256 x, UInt256 y, int *overflow) {
  return coerceUInt256 (mulUInt256 (x, y), overflow);
}

static int
tooBigUInt256 (UInt512 x) {
  return (0 != x.u64[4]
          || 0 != x.u64[5]
          || 0 != x.u64[6]
          || 0 != x.u64[7]);
}

extern UInt256
coerceUInt256 (UInt512  x, int *overflow) {
  assert (NULL != overflow);

  *overflow = tooBigUInt256(x);
  if (*overflow) return UINT256_ZERO;

  UInt256 result;
  result.u64[0] = x.u64[0];
  result.u64[1] = x.u64[1];
  result.u64[2] = x.u64[2];
  result.u64[3] = x.u64[3];
  return result;
}

extern int
compareUInt256 (UInt256 x, UInt256 y) {
  return (eqUInt256(x, y)
          ? 0
          : (gtUInt256(x, y)
             ? +1
             : -1));
}
