//
//  BBRUtilMath.h
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/10/2018.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Util_Math_H
#define BR_Util_Math_H

#include "support/BRInt.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LITTLE_ENDIAN != BYTE_ORDER
#error "Must be a `LITTLE ENDIAN` cpu architecture"
#endif

typedef enum {
  CORE_PARSE_OK,
  CORE_PARSE_STRANGE_DIGITS,    // non-decimals with optional single decimal point
  CORE_PARSE_UNDERFLOW,         // too many decimals for defined resolution .001 but need .01
  CORE_PARSE_OVERFLOW           // too many decimals for integer
} BRCoreParseStatus;

#define UINT256_INIT(value_as_uint64)   { .u64 = { (value_as_uint64), 0, 0, 0} }

/**
 * Create from a single uint64_t value.
 */
extern UInt256
createUInt256 (uint64_t value);

/**
 * Create as `(expt 10 power)` where power < 20 is required.
 */
extern UInt256
createUInt256Power (uint8_t power, int *overflow);

/**
 * Create as (expt 2 power) where power < 256
 */
extern UInt256
createUInt256Power2 (uint8_t power);

/**
 * Create from a string in the provided base.  The string must consist of only characters
 * in the base.  That is, avoid the '0x' prefix.  No decimal points; this is an integer parse.
 *
 * The string must be in big-endian format, always.  Generally this is only an issue for hex
 * (and perhaps) binary strings.  That is, for decimal strings you would expect "12.3" to some
 * who wind up with the number 3.21.  But for 0x0102, maybe not so clear.
 *
 * @param number - an integer value expressed in `base` in big-endian format.
 * @param base - must only be one of 2, 10, or 16.
 * @param pointer to a boolean error
 */
extern UInt256
createUInt256Parse (const char *number, int base, BRCoreParseStatus *status);

/**
 * Create from a string with the specified number of decimals (values after a decimal point).
 * Example w/ decimals = 5:
 *   "12"        -> 1200000
 *   "12.34"     -> 1234000
 *   "12.00001   -> 1200001
 *   "12.000001" -> 'error' (underflow)
 *    "0.00001"  ->       1
 *
 * @param number: a base10 number with one optional decimal point.
 * @param decimals: number of decimals after the decimal point
 * @param error: pointer to a boolean error.
 *
 * @warn Requires stack size of at least 3 * (strlen(number) + digits). [We are sloppy]
 */
extern UInt256
createUInt256ParseDecimal (const char *number, int decimals, BRCoreParseStatus *status);
  
  /**
 * Return `x + y`
 */
extern UInt512
addUInt256 (UInt256 x, UInt256 y);

/**
 * Return `x + y`.  If the result is too big then overflow is set to 1 and zero is returned.
 */
extern UInt256
addUInt256_Overflow (UInt256 x, UInt256 y, int *overflow);

/**
 * If `x >= y` return `x - y` and set `negative` to 0; otherwise, return `y - x` and set
 * `negative` to 1.
 */
extern UInt256
subUInt256_Negative (UInt256 x, UInt256 y, int *negative);

/**
 * Return `x * y`
 */
extern UInt512
mulUInt256 (UInt256 x, UInt256 y);

/**
 * Multiply as `x * y`.  If the result is too big then overflow is set to 1 and
 * zero is returned.
 */
extern UInt256
mulUInt256_Overflow (UInt256 x, UInt256 y, int *overflow);

/**
 * Multiply as `x * y` where `y` is a small (aka uint32) number.  If the result is too big
 * then overflow is set to 1 and zero is returned
 */
extern UInt256
mulUInt256_Small (UInt256 x, uint32_t y, int *overflow);

/**
 * Multiply as `x * y` where `y` is a positive double.  If `y` is negative, then this function
 * sets *negative to 1 and performs `x * -y`.  If the result is too big then overflow is set to 1
 * and zero is returned. `overflow` must not be NULL; `negative` must not be NULL.
 *
 * If `rem` is provided, then the remainder is returned.  Remainder occurs if you multiply,
 * for example, 1 * 0.123, rem will be 0.123.
 */
extern UInt256
mulUInt256_Double (UInt256 x, double y, int *overflow, int *negative, double *rem);

extern UInt256
divUInt256_Small (UInt256 x, uint32_t y, uint32_t *rem);

/**
 * Coerce `x`, a UInt512, to a UInt256.  If `x` is too big then overflow is set to 1 and
 * zero is returned.
 */
extern UInt256
coerceUInt256 (UInt512  x, int *overflow);

/**
 * Coerce `x`, a UInt256, to a uint64_t.  If `x` is too big then overflow is set to 1 and
 * zero is returned.
 */
extern uint64_t
coerceUInt64 (UInt256 x, int *overflow);

/**
 * Returns the string representation of `x` in `base`.  No matter the base, the returned string
 * will be in big-endian format.
 *
 * @warn YOU OWN THE RETURNED MEMORY
 */
extern char *
coerceString (UInt256 x, int base);

extern char *
coerceStringPrefaced (UInt256 x, int base, const char *preface);

/**
 * Returns a decimal string representing of `x` in base `0 with `decimals` digits after the
 * decimal-point.
 *
 * @warn YOU OWN THE RETURNED MEMORY
 */
extern char *
coerceStringDecimal (UInt256 x, int decimals);


/**
 * Returns a '0x' prefaced hex string
 */
extern char *
coerceUInt256HashToString (UInt256 hash);


//  static UInt256
//  divideUInt256 (UInt256 numerator, UInt256 denominator) {
//    // TODO: Implement
//    return UINT256_ZERO;
//  }
//
//  static UInt256
//  decodeUInt256 (const char *string) {
//    // TODO: Implement
//    return UINT256_ZERO;
//  }
//
//  static char *
//  encodeUInt256 (UInt256 value) {
//    // TODO: Implement
//    return NULL;
//  }

inline static int
eqUInt256 (UInt256 x, UInt256 y) {
  return UInt256Eq (x, y);
}

inline static int
gtUInt256 (UInt256 x, UInt256 y) {
  return x.u64[3] > y.u64[3]
  || (x.u64[3] == y.u64[3]
      && (x.u64[2] > y.u64[2]
          || (x.u64[2] == y.u64[2]
              && (x.u64[1] > y.u64[1]
                  || (x.u64[1] == y.u64[1]
                      && x.u64[0] > y.u64[0])))));
}

inline static int
geUInt256 (UInt256 x, UInt256 y) {
  return eqUInt256 (x, y) || gtUInt256 (x, y);
}

inline static int
ltUInt256 (UInt256 x, UInt256 y) {
  return x.u64[3] < y.u64[3]
  || (x.u64[3] == y.u64[3]
      && (x.u64[2] < y.u64[2]
          || (x.u64[2] == y.u64[2]
              && (x.u64[1] < y.u64[1]
                  || (x.u64[1] == y.u64[1]
                      && x.u64[0] < y.u64[0])))));
}

inline static int
leUInt256 (UInt256 x, UInt256 y) {
  return eqUInt256 (x, y) || ltUInt256 (x, y);
}

/**
 * Compare x and y returning:
 * -1 -> x  < y
 *  0 -> x == y
 * +1 -> x  > y
 */
extern int
compareUInt256 (UInt256 x, UInt256 y);

//
// Parsing
//
extern BRCoreParseStatus
parseIsInteger (const char *number);

extern BRCoreParseStatus
parseIsDecimal (const char *number);

#ifdef __cplusplus
}
#endif

#endif /* BR_Util_Math_H */
