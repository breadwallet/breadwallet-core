//
//  BBREthereumEther.c
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "BREthereumEther.h"

#if LITTLE_ENDIAN != BYTE_ORDER
#error "Must be a `LITTLE ENDIAN` cpu architecture"
#endif

//    > (define ether (lambda (x) (values (quotient x max64) (remainder x max64)))
//    > (ether #e1e21) -> 54, 3875820019684212736
//    > (ether #e1e24) -> 54210, 2003764205206896640
//    > (ether #e1e27) -> 54210108, 11515845246265065472
//    > (ether #e1e30) -> 54210108624, 5076944270305263616

static UInt256 etherUnitScaleFactor [NUMBER_OF_ETHER_UNITS] = {   /* LITTLE ENDIAN    */
        { .u64 = {                     1,            0, 0, 0 } }, /* wei       - 1    */
        { .u64 = {                  1000,            0, 0, 0 } }, /* kwei      - 1e3  */
        { .u64 = {               1000000,            0, 0, 0 } }, /* mwei      - 1e6  */
        { .u64 = {            1000000000,            0, 0, 0 } }, /* gwei      - 1e9  */
        { .u64 = {         1000000000000,            0, 0, 0 } }, /* szabo     - 1e12 */
        { .u64 = {      1000000000000000,            0, 0, 0 } }, /* finney    - 1e15 */
        { .u64 = {   1000000000000000000,            0, 0, 0 } }, /* ether     - 1e18 */
        { .u64 = {   3875820019684212736u,          54, 0, 0 } }, /* kether    - 1e21 */
        { .u64 = {   2003764205206896640u,       54210, 0, 0 } }, /* mether    - 1e24 */
        { .u64 = {  11515845246265065472u,    54210108, 0, 0 } }, /* gether    - 1e27 */
        { .u64 = {   5076944270305263616u, 54210108624, 0, 0 } }  /* tether    - 1e30 */
};

extern BREthereumEther
etherCreate(const UInt256 value) {
  BREthereumEther ether;
  ether.valueInWEI = value;
  return ether;
}

// TODO: Critical
extern BREthereumEther
etherCreateUnit(const UInt256 value, BREthereumEtherUnit unit, int *overflow) {
  assert (NULL != overflow);

  BREthereumEther ether;
  switch (unit) {
    case WEI:
      ether.valueInWEI = value;
      *overflow = 0;
      break;
    default: {
      ether.valueInWEI = mulUInt256_Overflow(value, etherUnitScaleFactor[unit], overflow);
      break;
    }
  }
  return ether;
}

extern BREthereumEther
etherCreateNumber (uint64_t number, BREthereumEtherUnit unit) {
  int overflow;
  UInt256 value = { .u64 = { number, 0, 0, 0 } };
  BREthereumEther ether = etherCreateUnit(value, unit, &overflow);
  assert (0 == overflow);
  return ether;
}

extern BREthereumEther
etherCreateZero(void) {
  return etherCreate(UINT256_ZERO);
}

//> (define max256 (expt 2 256))
//> (number->string max256)
//"115792089237316195423570985008687907853269984665640564039457584007913129639936"
//> (string-length (number->string max256))
//78
//
//> (number->string (expt 2 64))
//"18446744073709551616"
//> (string-length (number->string (expt 2 64)))
//20


#define MAX256_BASE10_LENGTH  80    // 78 + '\0' + luck.
#define MAX64_BASE10_LENGTH   19    // 19 digits => uint64_t w/o overflow

// Split `string` at '.' into `whole` and `fract` each of `size` (including `\0`)
static void
etherSplitString (const char *string, char *whole, char *fract, int size, BREthereumEtherParseStatus *status) {
  assert (size > 0); // at least one for '\0'

  // TODO: regex on [0-9\i\*
  whole[0] = '\0';
  fract[0] = '\0';
  *status = ETHEREUM_ETHER_PARSE_OK;

  if (0 == strlen (string)) return;

  char *dotFor = strchr(string, '.');
  char *dotRev = strrchr(string, '.');
  if (dotFor != dotRev) {  // only one '.'
    *status = ETHEREUM_ETHER_PARSE_STRANGE_DIGITS;
    return;
  }

  long wholeLength = (NULL == dotFor ? strlen(string) : dotFor - string);
  long fractLength = (NULL == dotFor ? 0 : (strlen(string) - wholeLength - 1));

  if (wholeLength > size - 1) {
    *status = ETHEREUM_ETHER_PARSE_OVERFLOW;
    return;
  }
  if (fractLength > size - 1) {
    *status =ETHEREUM_ETHER_PARSE_UNDERFLOW;    // TODO: sort of.
    return;
  }

  strncpy (whole, string, wholeLength);
  whole[wholeLength] = '\0';

  strncpy (fract, &string[wholeLength + 1], fractLength);
  fract[fractLength] = '\0';
  // trim trailing zeros
  for (int index = 1; index <= fractLength; index++) {
    if ('0' == fract[fractLength - index])
      fract[fractLength - index] = '\0';
    else
      break; // done on first non '0'
  }
}

// Convert the first `digits` characters from `string` into a UInt256.
static UInt256
etherCreatePartial (const char *string, int digits) {
  assert (digits <= MAX64_BASE10_LENGTH);

  char number[1 + MAX64_BASE10_LENGTH];
  strncpy (number, string, MAX64_BASE10_LENGTH);
  number[MAX64_BASE10_LENGTH] = '\0';

  uint64_t value = strtoull (number, NULL, 10);
  return createUInt256 (value);
}

static UInt256
etherScaleDigits (UInt256 value, int digits, int *overflow) {
  UInt256 scale = createUInt256Power(digits, overflow);
  return (*overflow
          ? UINT256_ZERO
          : mulUInt256_Overflow(value, scale, overflow));
}

static UInt256
etherCreateFull (const char *string, BREthereumEtherParseStatus *status) {
  UInt256 value = UINT256_ZERO;

  long length = strlen (string);
  for (long index = 0; index < length; index += MAX64_BASE10_LENGTH) {
    if (index == 0)
      value = etherCreatePartial(&string[index], MAX64_BASE10_LENGTH);
    else {
      int scaleOverflow = 0, addOverflow = 0;
      value = etherScaleDigits(value,
                               (length - index >= MAX64_BASE10_LENGTH ? MAX64_BASE10_LENGTH : (int) (length - index)),
                               &scaleOverflow);
      value = addUInt256_Overflow(value,
                                  etherCreatePartial(&string[index], MAX64_BASE10_LENGTH),
                                  &addOverflow);
      if (scaleOverflow || addOverflow) {
        *status = ETHEREUM_ETHER_PARSE_OVERFLOW;
        return UINT256_ZERO;
      }
    }

  }

  *status = ETHEREUM_ETHER_PARSE_OK;
  return value;
}

// Return the number of digits in a fraction.  "12.0003" -> "0003" -> 4
static int
etherFractionalDigits (const char *fraction) {
  return (int) strlen(fraction);
}

extern BREthereumEther
etherCreateString(const char *string, BREthereumEtherUnit unit, BREthereumEtherParseStatus *status) {
  int overflow = 0;

  char whole[MAX256_BASE10_LENGTH];  //
  char fract[MAX256_BASE10_LENGTH];

  // Only [0-9] and one decimal.
  etherSplitString (string, whole, fract, MAX256_BASE10_LENGTH, status);
  if (ETHEREUM_ETHER_PARSE_OK != *status)
    return etherCreateZero();

  // Process the 'whole' part.
  UInt256 wholeValue = etherCreateFull(whole, status);
  if (ETHEREUM_ETHER_PARSE_OK != *status)
    return etherCreateZero();

  // Process the 'fract' part.
  UInt256 fractValue = etherCreateFull(fract, status);
  if (ETHEREUM_ETHER_PARSE_OK != *status)
    return etherCreateZero();

  // If the fractional part is '0' we are done.
  if (eqUInt256(fractValue, UINT256_ZERO)) {
    BREthereumEther ether = etherCreateUnit(wholeValue, unit, &overflow);
    *status = (overflow
               ? ETHEREUM_ETHER_PARSE_OVERFLOW
               : ETHEREUM_ETHER_PARSE_OK);
    return ether;
  }

  // Check for underflow: ".02" WEI -> "20" (WEI-1) => underflow
  int fractDigits = etherFractionalDigits(fract);                 // ".02" -> 2
  // Every 3 fract digits is one ethereum unit
  int fractUnitOffset = fractDigits/3 + (0 != fractDigits%3);     // ".02" -> 0 + 1 -> 1
  // Shift the unit down by fractUnitOffset.  We'll scale 'whole' and 'fract' up accordingly.
  if (((int) unit) < fractUnitOffset) {
    *status = ETHEREUM_ETHER_PARSE_UNDERFLOW;
    return etherCreateZero();
  }
  else {
    unit -= fractUnitOffset;                                      // (unit - 1)
  }
  assert (unit >= 0);

  // Scale wholeValue by 10*digits
  int wholeScaleDigits = 3 * fractUnitOffset;                     // 3 => "12.02" -> "12000"
  int fractScaleDigits = 3 * fractUnitOffset - fractDigits;       // 1 =>   ".02" -> "020"

  int fractOverflow, wholeOverflow, sumOverflow;

  // Do the calculations assuming no overflow.
  wholeValue = etherScaleDigits(wholeValue, wholeScaleDigits, &wholeOverflow);
  fractValue = etherScaleDigits(fractValue, fractScaleDigits, &fractOverflow);

  UInt256 result = addUInt256_Overflow(wholeValue, fractValue, &sumOverflow);
  BREthereumEther ether = etherCreateUnit(result, unit, &overflow);

  if (wholeOverflow || fractOverflow || sumOverflow || overflow) {
    *status = ETHEREUM_ETHER_PARSE_OVERFLOW;
    return etherCreateZero();
  }

  *status = ETHEREUM_ETHER_PARSE_OK;
  return ether;
}

extern UInt256 // Can't be done: 1 WEI in ETHER... not representable as UInt256
etherGetValue(const BREthereumEther ether,
              BREthereumEtherUnit unit) {
  switch (unit) {
    case WEI:
      return ether.valueInWEI;
    default:
      // TODO: CRITICAL
      return UINT256_ZERO; /* divideUInt256 (ether.valueInWEI, etherUnitScaleFactor[unit]); */
  }
}

extern char * // Perhaps can be done. 1 WEI -> 1e-18 Ether
etherGetValueString(const BREthereumEther ether, BREthereumEtherUnit unit) {
  char *string = coerceString(ether.valueInWEI, 10);
  switch (unit) {
    case WEI:
      return string;
    default: {
      int decimals = 3 * unit;
      int slength = (int) strlen (string);
      if (decimals >= slength) {
        char *result = calloc (decimals + 3, 1);

        // Fill to decimals
        char format [10];
        sprintf (format, "0.%%%ds", decimals);
        sprintf (result, format, string);

        // Replace fills of ' ' with '0'
        for (int i = 0; i < decimals + 2; i++) {
          if (' ' == result[i])
            result[i] = '0';
        }
        return result;
      }
      else {
        int dindex = slength - decimals;
        char *result = calloc (slength + 2, 1);
        strncpy (result, string, dindex);
        result[dindex] = '.';
        strcpy (&result[dindex+1], &string[dindex]);
        return result;
      }
    }
  }
}

extern BRRlpItem
etherRlpEncode (const BREthereumEther ether, BRRlpCoder coder) {
  return rlpEncodeItemUInt256(coder, ether.valueInWEI);
}

extern BREthereumEther
etherAdd (BREthereumEther e1, BREthereumEther e2, int *overflow) {
  BREthereumEther result;
  result.valueInWEI = addUInt256_Overflow(e1.valueInWEI, e2.valueInWEI, overflow);
  return result;
}

extern BREthereumEther
etherSub (BREthereumEther e1, BREthereumEther e2, int *negative) {
  BREthereumEther result;
  result.valueInWEI = subUInt256_Negative(e1.valueInWEI, e2.valueInWEI, negative);
  return result;

}

//
// Comparisons
//
extern BREthereumBoolean
etherIsEQ (BREthereumEther e1, BREthereumEther e2) {
  return eqUInt256 (e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
etherIsGT (BREthereumEther e1, BREthereumEther e2) {
  return gtUInt256(e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
etherIsGE (BREthereumEther e1, BREthereumEther e2) {
  return geUInt256(e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
etherIsLT (BREthereumEther e1, BREthereumEther e2) {
  return ltUInt256(e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
etherIsLE (BREthereumEther e1, BREthereumEther e2) {
  return leUInt256(e1.valueInWEI, e2.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumBoolean
etherIsZero (BREthereumEther e) {
  return UInt256IsZero (e.valueInWEI) ? ETHEREUM_BOOLEAN_TRUE : ETHEREUM_BOOLEAN_FALSE;
}

extern BREthereumComparison
etherCompare (BREthereumEther e1, BREthereumEther e2) {
  switch (compareUInt256(e1.valueInWEI, e2.valueInWEI))
  {
    case -1: return ETHEREUM_COMPARISON_LT;
    case  0: return ETHEREUM_COMPARISON_EQ;
    case +1: return ETHEREUM_COMPARISON_GT;
    default: assert (0);
  }
}
