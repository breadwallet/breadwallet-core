//
//  testUtil.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "BRUtil.h"

//
// Math Tests
//
static void
runMathAddTests () {
    int carry = -1;
    UInt256 z;
    UInt256 x0atMax = { .u32 = { UINT32_MAX, 0, 0, 0, 0, 0, 0, 0 }};
    UInt256 xOne    = { .u32 = {          1, 0, 0, 0, 0, 0, 0, 0 }};
    UInt256 xTwo    = { .u32 = {          2, 0, 0, 0, 0, 0, 0, 0 }};
    UInt256 x2to32  = { .u32 = {          0, 1, 0, 0, 0, 0, 0, 0 }};
    UInt256 x7atMax = { .u32 = {          0, 0, 0, 0, 0, 0, 0, UINT32_MAX }};
    UInt256 x7atOne = { .u32 = {          0, 0, 0, 0, 0, 0, 0, 1 }};
    UInt256 xMax    = { .u32 = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX }};

    z = addUInt256_Overflow (xOne, xOne, &carry);
    assert (2 == z.u32[0] && 0 == carry);

    z = addUInt256_Overflow (xOne, x0atMax, &carry);
    assert (1 == z.u32[1] && 0 == carry);

    z = addUInt256_Overflow (xOne, x2to32, &carry);
    assert (1 == z.u32[0] && 1 == z.u32[1] && 0 == carry);

    z = addUInt256_Overflow (xTwo, x7atOne, &carry);
    assert (2 == z.u32[0] && 1 == z.u32[7] && 0 == carry);

    z = addUInt256_Overflow (x7atMax, z, &carry);
    assert (0 == z.u32[7] && 0 == z.u32[0] && 1 == carry);

    z = addUInt256_Overflow (xMax, xOne, &carry);
    assert (1 == carry);
}

static void
runMathSubTests () {
    int negative = -1;
    UInt256 z;
    UInt256 x0atMax = { .u32 = { UINT32_MAX, 0, 0, 0, 0, 0, 0, 0 }};
    UInt256 xOne    = { .u32 = {          1, 0, 0, 0, 0, 0, 0, 0 }};
    UInt256 xOneOne = { .u32 = {          1, 1, 0, 0, 0, 0, 0, 0 }};
    UInt256 xTwo    = { .u32 = {          2, 0, 0, 0, 0, 0, 0, 0 }};
    //  UInt256 x2to32  = { .u32 = {          0, 1, 0, 0, 0, 0, 0, 0 }};
    //  UInt256 x7atMax = { .u32 = {          0, 0, 0, 0, 0, 0, 0, UINT32_MAX }};
    //  UInt256 x7atOne = { .u32 = {          0, 0, 0, 0, 0, 0, 0, 1 }};
    //  UInt256 xMax    = { .u32 = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX }};

    z = subUInt256_Negative(xOne, xOne, &negative);
    assert (0 == z.u32[0] && 0 == negative);

    z = subUInt256_Negative(xTwo, xOne, &negative);
    assert (1 == z.u32[0] && 0 == negative);

    z = subUInt256_Negative(xOne, xTwo, &negative);
    assert (1 == z.u32[0] && 1 == negative);

    z = subUInt256_Negative(xOne, x0atMax, &negative);
    assert ((UINT32_MAX - 1) == z.u32[0]
            && 0 == z.u32[7]
            && 1 == negative);

    z = subUInt256_Negative(xOneOne, xTwo, &negative);
    UInt256 zr5 = { .u32 = { UINT32_MAX, 0, 0, 0, 0, 0, 0, 0 }};
    assert (eqUInt256(zr5, z) && 0 == negative);
    assert (UINT32_MAX == z.u32[0]
            && 0 == z.u32[1]
            && 0 == z.u32[2]
            && 0 == z.u32[7]
            && 0 == negative);
}

static void
runMathMulTests () {
    UInt512 z;
    UInt256 x0atMax   = { .u32 = { UINT32_MAX, 0, 0, 0, 0, 0, 0, 0 }};
    UInt256 xOne      = { .u32 = {          1, 0, 0, 0, 0, 0, 0, 0 }};
    UInt256 xTwo      = { .u32 = {          2, 0, 0, 0, 0, 0, 0, 0 }};
    UInt256 x2to31    = { .u32 = {    (1<<31), 0, 0, 0, 0, 0, 0, 0 }};
    UInt256 x2to32    = { .u32 = {          0, 1, 0, 0, 0, 0, 0, 0 }};
    UInt256 x7atOne   = { .u32 = {          0, 0, 0, 0, 0, 0, 0, 1 }};
    UInt256 x7at2to31 = { .u32 = {          0, 0, 0, 0, 0, 0, 0, (1<<31) }};
    UInt256 xMax      = { .u32 = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX }};

    //  > (define xMax (- (expt 2 256) 1)
    //  > (number->string xMax 2)
    //  "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"

    z = mulUInt256(xOne, x0atMax);
    assert (UINT32_MAX == z.u32[0] && 0 == z.u32[1] /* && ...*/);

    z = mulUInt256(xOne, x7atOne);
    assert (1 == z.u32[7]);

    z = mulUInt256(x2to31, xTwo);
    assert (0 == z.u32[0] && 1 == z.u32[1] && 0 == z.u32[2] /* && ... */);

    z = mulUInt256(x2to32, x2to32);
    assert (0 == z.u32[0] && 0 == z.u32[1] && 1 == z.u32[2] && 0 == z.u32[3]);

    // (= (* (expt 2 255) (expt 2 255)) (expt 2 510))
    z = mulUInt256(x7at2to31, x7at2to31);
    assert ((1<<30) == z.u32[15]);

    z = mulUInt256(xMax, xMax);
    //  > (number->string (* xMax xMax) 2)
    //  "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111110 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001"
    //  define (factor m)
    //    (if (positive? m)
    //        (begin
    //         (display (remainder m (expt 2 32))) (newline)
    //         (factor (quotient m (expt 2 32))))))
    //  > (factor (* m256 m256))
    //  1
    //  0
    //  0
    //  0
    //  0
    //  0
    //  0
    //  0
    //  4294967294
    //  4294967295
    //  4294967295
    //  4294967295
    //  4294967295
    //  4294967295
    //  4294967295
    //  4294967295
    assert (1 == z.u32[0]
            && 0 == z.u32[1]
            // ...
            && 0 == z.u32[7]
            && UINT32_MAX - 1 == z.u32[ 8]
            && UINT32_MAX == z.u32[ 9]
            && UINT32_MAX == z.u32[10]
            && UINT32_MAX == z.u32[11]
            && UINT32_MAX == z.u32[12]
            && UINT32_MAX == z.u32[13]
            && UINT32_MAX == z.u32[14]
            && UINT32_MAX == z.u32[15]);

}

static void
runMathMulDoubleTests () {
    BRCoreParseStatus status;
    int over, neg; double rem;
    UInt256 ai, ao, r;

    ai = createUInt256Parse("1000000000000000", 10, &status);  // Input
    ao = createUInt256Parse("1000000000000000", 10, &status);  // Output
    r = mulUInt256_Double(ai, -1.0, &over, &neg, &rem);
    assert (over == 0 && neg == 1 && eqUInt256(r, ao));
    assert (0 == strcmp ("1000000000000000", coerceString(r, 10)));

    ai = createUInt256Parse("1000000000000000", 10, &status);  // Input
    ao = createUInt256Parse(  "10000000000000", 10, &status);  // Output
    r = mulUInt256_Double(ai, 0.01, &over, &neg, &rem);
    assert (over == 0 && neg == 0 && eqUInt256(r, ao));
    assert (0 == strcmp ("10000000000000", coerceString(r, 10)));

    ai = createUInt256Parse("1000000000000000", 10, &status);  // Input
    ao = createUInt256Parse("10000000", 10, &status);  // Output
    r = mulUInt256_Double(ai, 0.00000001, &over, &neg, &rem);
    assert (over == 0 && neg == 0 && eqUInt256(r, ao));
    assert (0 == strcmp ("10000000", coerceString(r, 10)));

    ai = createUInt256Parse("1000000000000000", 10, &status);  // Input
    ao = createUInt256Parse("100000000000000000000", 10, &status);  // Output
    r = mulUInt256_Double(ai, 100000.0, &over, &neg, &rem);
    assert (over == 0 && neg == 0 && eqUInt256(r, ao));
    assert (0 == strcmp ("100000000000000000000", coerceString(r, 10)));


    ai = createUInt256Parse("1000000000000000", 10, &status);  // Input
    ao = createUInt256Parse("100000000000000000000000000000000000", 10, &status);  // Output
    r = mulUInt256_Double(ai, 10000000000.0, &over, &neg, &rem);
    r = mulUInt256_Double(r,  10000000000.0, &over, &neg, &rem);
    assert (over == 0 && neg == 0 && eqUInt256(r, ao));
    assert (0 == strcmp ("100000000000000000000000000000000000", coerceString(r, 10)));

    ai = createUInt256Parse("1", 10, &status);  // Input
    ao = createUInt256Parse("0", 10, &status);  // Output
    r = mulUInt256_Double(ai, 0.123, &over, &neg, &rem);
    assert (over == 0 && eqUInt256(r, ao));
    assert (0 == strcmp ("0", coerceString(r, 10)));
    assert (0.123 == rem);

    ai = createUInt256Parse("2000000000000000000", 10, &status);  // Output
    ao = createUInt256 (2);
    r  = mulUInt256_Double(ai, 1e-18, &over, &neg, &rem);
    assert (over == 0 && eqUInt256(r, ao));
    assert (0 == strcmp ("2", coerceString(r, 10)));

    // overflow...
}

static void
runMathDivTests () {
    UInt256 r = UINT256_ZERO;
    uint32_t rem;

    // 10^21
    r.u64[0] = 3875820019684212736u;
    r.u64[1] = 54;

    // 10^15
    UInt256 a = divUInt256_Small(r, 1000000, &rem);
    assert (0 == rem
            && a.u64[0] == 1000000000000000
            && a.u64[1] == 0
            && a.u64[2] == 0
            && a.u64[3] == 0);
}

static void
runMathCoerceTests () {
    UInt256 a = { .u64 = { 1000000000000000, 0, 0, 0}};
    const char *as = coerceString(a, 10);
    assert (0 == strcmp (as, "1000000000000000"));
    free ((char *) as);

    UInt256 b = { .u64 = { 3875820019684212736u, 54, 0, 0}};
    const char *bs = coerceString(b, 10);
    assert (0 == strcmp (bs, "1000000000000000000000"));
    free ((char *) bs);

    UInt256 c = { .u64 = { 15, 0, 0, 0}};
    const char *cs = coerceString(c, 2);
    assert (0 == strcmp (cs, "00001111"));  // unexpected
    free ((char *) cs);

    UInt256 d = { .u64 = { 0, UINT64_MAX, 0, 0}};
    const char *ds = coerceString(d, 2);
    assert (0 == strcmp (ds, "11111111111111111111111111111111111111111111111111111111111111110000000000000000000000000000000000000000000000000000000000000000"));
    free ((char *) ds);

    UInt256 e = { .u64 = { 15, 0, 0, 0}};
    const char *es = coerceString(e, 16);
    assert (0 == strcmp (es, "0f"));  // unexpected
    free ((char *) es);

}

static void
runMathParseTests () {
    BRCoreParseStatus status;
    UInt256 r = UINT256_ZERO;
    UInt256 a = UINT256_ZERO;

    assert (CORE_PARSE_OK == parseIsInteger("0"));
    assert (CORE_PARSE_OK == parseIsInteger("0123456789"));
    assert (CORE_PARSE_OK != parseIsInteger("0123456789."));
    assert (CORE_PARSE_OK != parseIsInteger(""));
    assert (CORE_PARSE_OK != parseIsInteger("."));
    assert (CORE_PARSE_OK != parseIsInteger("1."));
    assert (CORE_PARSE_OK != parseIsInteger(".0"));
    assert (CORE_PARSE_OK != parseIsInteger("a"));

    assert (CORE_PARSE_OK == parseIsDecimal ("1"));
    assert (CORE_PARSE_OK == parseIsDecimal ("1."));
    assert (CORE_PARSE_OK == parseIsDecimal ("1.1"));
    assert (CORE_PARSE_OK == parseIsDecimal ("0.12"));
    assert (CORE_PARSE_OK != parseIsDecimal (NULL));
    assert (CORE_PARSE_OK != parseIsDecimal (""));
    assert (CORE_PARSE_OK != parseIsDecimal (".12"));
    assert (CORE_PARSE_OK != parseIsDecimal ("0.12."));
    assert (CORE_PARSE_OK != parseIsDecimal ("0.12.34"));
    assert (CORE_PARSE_OK != parseIsDecimal ("a"));


    assert (1 == encodeHexValidate("ab"));
    assert (1 == encodeHexValidate("ab01"));
    assert (1 != encodeHexValidate(NULL));
    assert (1 != encodeHexValidate(""));
    assert (1 != encodeHexValidate("0"));
    assert (1 != encodeHexValidate("f"));
    assert (1 != encodeHexValidate("ff0"));
    assert (1 != encodeHexValidate("1g"));


    // "0x09184e72a000" // 10000000000000
    r = createUInt256Parse("09184e72a000", 16, &status);
    a.u64[0] = 10000000000000;
    assert (CORE_PARSE_OK == status && eqUInt256(r, a));

    // "0x0234c8a3397aab58" // 158972490234375000
    r = createUInt256Parse("0234c8a3397aab58", 16, &status);
    a.u64[0] = 158972490234375000;
    assert (CORE_PARSE_OK == status && eqUInt256(r, a));

    // 115792089237316195423570985008687907853269984665640564039457584007913129639935
    r = createUInt256Parse("115792089237316195423570985008687907853269984665640564039457584007913129639935", 10, &status);
    a.u64[0] = a.u64[1] = a.u64[2] = a.u64[3] = UINT64_MAX;
    assert (CORE_PARSE_OK == status && eqUInt256(r, a));

    r = createUInt256Parse("1000000000000000000000000000000", 10, &status); // 1 TETHER (10^30)
    assert (CORE_PARSE_OK == status
            && r.u64[0] == 5076944270305263616u
            && r.u64[1] == 54210108624
            && r.u64[2] == 0
            && r.u64[3] == 0);

    char *s;
    r = createUInt256Parse("425693205796080237694414176550132631862392541400559", 10, &status);
    s = coerceString(r, 10);
    assert (0 == strcmp("425693205796080237694414176550132631862392541400559", s));
    free (s);

    s = coerceString(r, 16);
    printf ("S: %s\n", s);
    assert (0 == strcasecmp("0123456789ABCDEFEDCBA98765432123456789ABCDEF", s));
    free(s);

    r = createUInt256Parse("0123456789ABCDEFEDCBA98765432123456789ABCDEF", 16, &status);
    s = coerceString(r, 16);
    assert (0 == strcasecmp ("0123456789ABCDEFEDCBA98765432123456789ABCDEF", s));
    free (s);

    s = coerceString(r, 10);
    assert (0 == strcmp ("425693205796080237694414176550132631862392541400559", s));
    free (s);

    r = UINT256_ZERO;
    s = coerceString(r, 10);
    assert (0 == strcmp ("0", s));
    free (s);

    s = coerceString(r, 16);
    assert (0 == strcmp ("0", s));
    free (s);

    r = createUInt256Parse("596877", 10, &status);
    s = coerceString(r, 16);
    printf ("596877: %s\n", s);

    r = createUInt256Parse
    ("1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
     2, &status);
    assert (CORE_PARSE_OK == status && UINT64_MAX == r.u64[0] && UINT64_MAX == r.u64[1] && UINT64_MAX == r.u64[2] && UINT64_MAX == r.u64[3]);

    r = createUInt256Parse ("ffffffffffffffff", 16, &status);
    assert (CORE_PARSE_OK == status && UINT64_MAX == r.u64[0] && 0 == r.u64[1] && 0 == r.u64[2] && 0 == r.u64[3]);

    r = createUInt256Parse ("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 16, &status);
    assert (CORE_PARSE_OK == status && UINT64_MAX == r.u64[0] && UINT64_MAX == r.u64[1] && UINT64_MAX == r.u64[2] && UINT64_MAX == r.u64[3]);

    r = createUInt256ParseDecimal(".01", 2, &status);
    assert (CORE_PARSE_OK != status);

    r = createUInt256ParseDecimal("0.01", 2, &status);
    assert (CORE_PARSE_OK == status && 1 == r.u64[0] && 0 == r.u64[1] && 0 == r.u64[2] && 0 == r.u64[3]);

    r = createUInt256ParseDecimal("01.", 0, &status);
    assert (CORE_PARSE_OK == status && 1 == r.u64[0] && 0 == r.u64[1] && 0 == r.u64[2] && 0 == r.u64[3]);

    r = createUInt256ParseDecimal("01.", 2, &status);
    assert (CORE_PARSE_OK == status && 100 == r.u64[0] && 0 == r.u64[1] && 0 == r.u64[2] && 0 == r.u64[3]);

    r = createUInt256ParseDecimal("1", 2, &status);
    assert (CORE_PARSE_OK == status && 100 == r.u64[0] && 0 == r.u64[1] && 0 == r.u64[2] && 0 == r.u64[3]);

    r = createUInt256ParseDecimal("2.5", 0, &status);
    assert (CORE_PARSE_UNDERFLOW == status);
}

extern void
runUtilTests (void) {
    runMathParseTests ();
    runMathCoerceTests();
    runMathAddTests();
    runMathSubTests();
    runMathMulTests();
    runMathMulDoubleTests();
    runMathDivTests();
}
