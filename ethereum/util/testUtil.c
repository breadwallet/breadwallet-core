//
//  testUtil.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "ethereum/util/BRUtil.h"

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

    z = uint256Add_Overflow (xOne, xOne, &carry);
    assert (2 == z.u32[0] && 0 == carry);

    z = uint256Add_Overflow (xOne, x0atMax, &carry);
    assert (1 == z.u32[1] && 0 == carry);

    z = uint256Add_Overflow (xOne, x2to32, &carry);
    assert (1 == z.u32[0] && 1 == z.u32[1] && 0 == carry);

    z = uint256Add_Overflow (xTwo, x7atOne, &carry);
    assert (2 == z.u32[0] && 1 == z.u32[7] && 0 == carry);

    z = uint256Add_Overflow (x7atMax, z, &carry);
    assert (0 == z.u32[7] && 0 == z.u32[0] && 1 == carry);

    z = uint256Add_Overflow (xMax, xOne, &carry);
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

    z = uint256Sub_Negative(xOne, xOne, &negative);
    assert (0 == z.u32[0] && 0 == negative);

    z = uint256Sub_Negative(xTwo, xOne, &negative);
    assert (1 == z.u32[0] && 0 == negative);

    z = uint256Sub_Negative(xOne, xTwo, &negative);
    assert (1 == z.u32[0] && 1 == negative);

    z = uint256Sub_Negative(xOne, x0atMax, &negative);
    assert ((UINT32_MAX - 1) == z.u32[0]
            && 0 == z.u32[7]
            && 1 == negative);

    z = uint256Sub_Negative(xOneOne, xTwo, &negative);
    UInt256 zr5 = { .u32 = { UINT32_MAX, 0, 0, 0, 0, 0, 0, 0 }};
    assert (uint256EQL(zr5, z) && 0 == negative);
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

    z = uint256Mul(xOne, x0atMax);
    assert (UINT32_MAX == z.u32[0] && 0 == z.u32[1] /* && ...*/);

    z = uint256Mul(xOne, x7atOne);
    assert (1 == z.u32[7]);

    z = uint256Mul(x2to31, xTwo);
    assert (0 == z.u32[0] && 1 == z.u32[1] && 0 == z.u32[2] /* && ... */);

    z = uint256Mul(x2to32, x2to32);
    assert (0 == z.u32[0] && 0 == z.u32[1] && 1 == z.u32[2] && 0 == z.u32[3]);

    // (= (* (expt 2 255) (expt 2 255)) (expt 2 510))
    z = uint256Mul(x7at2to31, x7at2to31);
    assert ((1<<30) == z.u32[15]);

    z = uint256Mul(xMax, xMax);
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
    int over, neg; double rem, v;
    UInt256 ai, ao, r;

    ai = uint256CreateParse("1000000000000000", 10, &status);  // Input
    ao = uint256CreateParse("1000000000000000", 10, &status);  // Output
    r = uint256Mul_Double(ai, -1.0, &over, &neg, &rem);
    assert (over == 0 && neg == 1 && uint256EQL(r, ao));
    assert (0 == strcmp ("1000000000000000", uint256CoerceString(r, 10)));

    ai = uint256CreateParse("1000000000000000", 10, &status);  // Input
    ao = uint256CreateParse(  "10000000000000", 10, &status);  // Output
    r = uint256Mul_Double(ai, 0.01, &over, &neg, &rem);
    assert (over == 0 && neg == 0 && uint256EQL(r, ao));
    assert (0 == strcmp ("10000000000000", uint256CoerceString(r, 10)));

    ai = uint256CreateParse("1000000000000000", 10, &status);  // Input
    ao = uint256CreateParse("10000000", 10, &status);  // Output
    r = uint256Mul_Double(ai, 0.00000001, &over, &neg, &rem);
    assert (over == 0 && neg == 0 && uint256EQL(r, ao));
    assert (0 == strcmp ("10000000", uint256CoerceString(r, 10)));

    ai = uint256CreateParse("1000000000000000", 10, &status);  // Input
    ao = uint256CreateParse("100000000000000000000", 10, &status);  // Output
    r = uint256Mul_Double(ai, 100000.0, &over, &neg, &rem);
    assert (over == 0 && neg == 0 && uint256EQL(r, ao));
    assert (0 == strcmp ("100000000000000000000", uint256CoerceString(r, 10)));


    ai = uint256CreateParse("1000000000000000", 10, &status);  // Input
    ao = uint256CreateParse("100000000000000000000000000000000000", 10, &status);  // Output
    r = uint256Mul_Double(ai, 10000000000.0, &over, &neg, &rem);
    r = uint256Mul_Double(r,  10000000000.0, &over, &neg, &rem);
    assert (over == 0 && neg == 0 && uint256EQL(r, ao));
    assert (0 == strcmp ("100000000000000000000000000000000000", uint256CoerceString(r, 10)));

    ai = uint256CreateParse("1", 10, &status);  // Input
    ao = uint256CreateParse("0", 10, &status);  // Output
    r = uint256Mul_Double(ai, 0.123, &over, &neg, &rem);
    assert (over == 0 && uint256EQL(r, ao));
    assert (0 == strcmp ("0", uint256CoerceString(r, 10)));
    assert (0.123 == rem);

    ai = uint256CreateParse("2000000000000000000", 10, &status);  // Output
    ao = uint256Create (2);
    r  = uint256Mul_Double(ai, 1e-18, &over, &neg, &rem);
    assert (over == 0 && uint256EQL(r, ao));
    assert (0 == strcmp ("2", uint256CoerceString(r, 10)));

    double x = 25.25434525155732538797258871;
    r = uint256CreateDouble(x, 18, &over);
    assert (!over && !uint256EQL(r, UINT256_ZERO));
    v  = uint256CoerceDouble(r, &over);
    assert(!over && fabs(v*1e-18 - x) / x < 1e-10);
}

static void
runMathDivTests () {
    UInt256 r = UINT256_ZERO;
    uint32_t rem;

    // 10^21
    r.u64[0] = 3875820019684212736u;
    r.u64[1] = 54;

    // 10^15
    UInt256 a = uint256Div_Small(r, 1000000, &rem);
    assert (0 == rem
            && a.u64[0] == 1000000000000000
            && a.u64[1] == 0
            && a.u64[2] == 0
            && a.u64[3] == 0);
}

static void
runMathCoerceTests () {
    UInt256 a = { .u64 = { 1000000000000000, 0, 0, 0}};
    const char *as = uint256CoerceString(a, 10);
    assert (0 == strcmp (as, "1000000000000000"));
    free ((char *) as);

    UInt256 b = { .u64 = { 3875820019684212736u, 54, 0, 0}};
    const char *bs = uint256CoerceString(b, 10);
    assert (0 == strcmp (bs, "1000000000000000000000"));
    free ((char *) bs);

    UInt256 c = { .u64 = { 15, 0, 0, 0}};
    const char *cs = uint256CoerceString(c, 2);
    assert (0 == strcmp (cs, "00001111"));  // unexpected
    free ((char *) cs);

    UInt256 d = { .u64 = { 0, UINT64_MAX, 0, 0}};
    const char *ds = uint256CoerceString(d, 2);
    assert (0 == strcmp (ds, "11111111111111111111111111111111111111111111111111111111111111110000000000000000000000000000000000000000000000000000000000000000"));
    free ((char *) ds);

    UInt256 e = { .u64 = { 15, 0, 0, 0}};
    const char *es = uint256CoerceString(e, 16);
    assert (0 == strcmp (es, "0f"));  // unexpected
    free ((char *) es);

    // Value: 0, w/ and w/o a prefix
    UInt256 f = { .u64 = { 0, 0, 0, 0}};
    const char *fs = uint256CoerceString (f, 10);
    assert (0 == strcmp (fs, "0"));
    free ((char *) fs);

    fs = uint256CoerceStringPrefaced (f, 10, NULL);
    assert (0 == strcmp (fs, "0"));
    free ((char *) fs);

    fs = uint256CoerceStringPrefaced (f, 10, "");
    assert (0 == strcmp (fs, "0"));
    free ((char *) fs);

    fs = uint256CoerceStringPrefaced (f, 10, "hex");
    assert (0 == strcmp (fs, "hex0"));
    free ((char *) fs);

    fs = uint256CoerceStringPrefaced (f, 16, "0x");
    assert (0 == strcmp (fs, "0x0"));
    free ((char *) fs);

    UInt256 g = uint256Create(0x0a);
    const char *gs = uint256CoerceString (g, 10);
    assert (0 == strcmp (gs, "10"));
    free ((char *) gs);

    gs = uint256CoerceStringPrefaced (g, 10, NULL);
    assert (0 == strcmp (gs, "10"));
    free ((char *) gs);

    gs = uint256CoerceStringPrefaced (g, 16, "0x");
    assert (0 == strcmp (gs, "0xa"));
    free ((char *) gs);
}

static void
runMathParseTests () {
    BRCoreParseStatus status;
    UInt256 r = UINT256_ZERO;
    UInt256 a = UINT256_ZERO;

    assert (CORE_PARSE_OK == stringParseIsInteger("0"));
    assert (CORE_PARSE_OK == stringParseIsInteger("0123456789"));
    assert (CORE_PARSE_OK != stringParseIsInteger("0123456789."));
    assert (CORE_PARSE_OK != stringParseIsInteger(""));
    assert (CORE_PARSE_OK != stringParseIsInteger("."));
    assert (CORE_PARSE_OK != stringParseIsInteger("1."));
    assert (CORE_PARSE_OK != stringParseIsInteger(".0"));
    assert (CORE_PARSE_OK != stringParseIsInteger("a"));

    assert (CORE_PARSE_OK == stringParseIsDecimal ("1"));
    assert (CORE_PARSE_OK == stringParseIsDecimal ("1."));
    assert (CORE_PARSE_OK == stringParseIsDecimal ("1.1"));
    assert (CORE_PARSE_OK == stringParseIsDecimal ("0.12"));
    assert (CORE_PARSE_OK != stringParseIsDecimal (NULL));
    assert (CORE_PARSE_OK != stringParseIsDecimal (""));
    assert (CORE_PARSE_OK != stringParseIsDecimal (".12"));
    assert (CORE_PARSE_OK != stringParseIsDecimal ("0.12."));
    assert (CORE_PARSE_OK != stringParseIsDecimal ("0.12.34"));
    assert (CORE_PARSE_OK != stringParseIsDecimal ("a"));


    assert (1 == hexEncodeValidate("ab"));
    assert (1 == hexEncodeValidate("ab01"));
    assert (1 != hexEncodeValidate(NULL));
    assert (1 != hexEncodeValidate(""));
    assert (1 != hexEncodeValidate("0"));
    assert (1 != hexEncodeValidate("f"));
    assert (1 != hexEncodeValidate("ff0"));
    assert (1 != hexEncodeValidate("1g"));


    // "0x09184e72a000" // 10000000000000
    r = uint256CreateParse("09184e72a000", 16, &status);
    a.u64[0] = 10000000000000;
    assert (CORE_PARSE_OK == status && uint256EQL(r, a));

    // "0x0234c8a3397aab58" // 158972490234375000
    r = uint256CreateParse("0234c8a3397aab58", 16, &status);
    a.u64[0] = 158972490234375000;
    assert (CORE_PARSE_OK == status && uint256EQL(r, a));

    // 115792089237316195423570985008687907853269984665640564039457584007913129639935
    r = uint256CreateParse("115792089237316195423570985008687907853269984665640564039457584007913129639935", 10, &status);
    a.u64[0] = a.u64[1] = a.u64[2] = a.u64[3] = UINT64_MAX;
    assert (CORE_PARSE_OK == status && uint256EQL(r, a));

    r = uint256CreateParse("1000000000000000000000000000000", 10, &status); // 1 TETHER (10^30)
    assert (CORE_PARSE_OK == status
            && r.u64[0] == 5076944270305263616u
            && r.u64[1] == 54210108624
            && r.u64[2] == 0
            && r.u64[3] == 0);

    r = uint256CreateParse("0000", 10, &status);
    assert (CORE_PARSE_OK == status && uint256EQL (r, UINT256_ZERO));

    r = uint256CreateParse("0000", 2, &status);
    assert (CORE_PARSE_OK == status && uint256EQL (r, UINT256_ZERO));

    r = uint256CreateParse("0x0000", 16, &status);
    assert (CORE_PARSE_OK == status && uint256EQL (r, UINT256_ZERO));

    r = uint256CreateParse("0x", 16, &status);
    assert (CORE_PARSE_OK == status && uint256EQL (r, UINT256_ZERO));

    r = uint256CreateParse("", 10, &status);
    assert (CORE_PARSE_OK == status && uint256EQL (r, UINT256_ZERO));

    r = uint256CreateParse("", 2, &status);
    assert (CORE_PARSE_OK == status && uint256EQL (r, UINT256_ZERO));

    r = uint256CreateParse("", 16, &status);
    assert (CORE_PARSE_OK == status && uint256EQL (r, UINT256_ZERO));


    char *s;
    r = uint256CreateParse("425693205796080237694414176550132631862392541400559", 10, &status);
    s = uint256CoerceString(r, 10);
    assert (0 == strcmp("425693205796080237694414176550132631862392541400559", s));
    free (s);

    s = uint256CoerceString(r, 16);
    printf ("S: %s\n", s);
    assert (0 == strcasecmp("0123456789ABCDEFEDCBA98765432123456789ABCDEF", s));
    free(s);

    r = uint256CreateParse("0123456789ABCDEFEDCBA98765432123456789ABCDEF", 16, &status);
    s = uint256CoerceString(r, 16);
    assert (0 == strcasecmp ("0123456789ABCDEFEDCBA98765432123456789ABCDEF", s));
    free (s);

    s = uint256CoerceString(r, 10);
    assert (0 == strcmp ("425693205796080237694414176550132631862392541400559", s));
    free (s);

    r = UINT256_ZERO;
    s = uint256CoerceString(r, 10);
    assert (0 == strcmp ("0", s));
    free (s);

    s = uint256CoerceString(r, 16);
    assert (0 == strcmp ("0", s));
    free (s);

    r = uint256CreateParse("596877", 10, &status);
    s = uint256CoerceString(r, 16);
    printf ("596877: %s\n", s);

    r = uint256CreateParse
    ("1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
     2, &status);
    assert (CORE_PARSE_OK == status && UINT64_MAX == r.u64[0] && UINT64_MAX == r.u64[1] && UINT64_MAX == r.u64[2] && UINT64_MAX == r.u64[3]);

    r = uint256CreateParse ("ffffffffffffffff", 16, &status);
    assert (CORE_PARSE_OK == status && UINT64_MAX == r.u64[0] && 0 == r.u64[1] && 0 == r.u64[2] && 0 == r.u64[3]);

    r = uint256CreateParse ("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff", 16, &status);
    assert (CORE_PARSE_OK == status && UINT64_MAX == r.u64[0] && UINT64_MAX == r.u64[1] && UINT64_MAX == r.u64[2] && UINT64_MAX == r.u64[3]);

    r = uint256CreateParseDecimal(".01", 2, &status);
    assert (CORE_PARSE_OK != status);

    r = uint256CreateParseDecimal("0.01", 2, &status);
    assert (CORE_PARSE_OK == status && 1 == r.u64[0] && 0 == r.u64[1] && 0 == r.u64[2] && 0 == r.u64[3]);

    r = uint256CreateParseDecimal("01.", 0, &status);
    assert (CORE_PARSE_OK == status && 1 == r.u64[0] && 0 == r.u64[1] && 0 == r.u64[2] && 0 == r.u64[3]);

    r = uint256CreateParseDecimal("01.", 2, &status);
    assert (CORE_PARSE_OK == status && 100 == r.u64[0] && 0 == r.u64[1] && 0 == r.u64[2] && 0 == r.u64[3]);

    r = uint256CreateParseDecimal("1", 2, &status);
    assert (CORE_PARSE_OK == status && 100 == r.u64[0] && 0 == r.u64[1] && 0 == r.u64[2] && 0 == r.u64[3]);

    r = uint256CreateParseDecimal("2.5", 0, &status);
    assert (CORE_PARSE_UNDERFLOW == status);


    // Strings for: 0xa

    r = uint256CreateParse("0xa", 16, &status);
    s = uint256CoerceStringPrefaced(r, 16, "0x");
    assert (0 == strcmp ("0xa", s));
    free (s);

    r = uint256CreateParse("0x0a", 16, &status);
    s = uint256CoerceStringPrefaced(r, 16, "0x");
    assert (0 == strcmp ("0xa", s));
    free (s);

    r = uint256CreateParse("0x00a", 16, &status);
    s = uint256CoerceStringPrefaced(r, 16, "0x");
    assert (0 == strcmp ("0xa", s));
    free (s);

    // Strings for 0x0

    r = uint256CreateParse("0x", 16, &status);
    s = uint256CoerceStringPrefaced(r, 16, "0x");
    assert (0 == strcmp ("0x0", s));
    free (s);

    r = uint256CreateParse("0x0", 16, &status);
    s = uint256CoerceStringPrefaced(r, 16, "0x");
    assert (0 == strcmp ("0x0", s));
    free (s);

    r = uint256CreateParse("0x00", 16, &status);
    s = uint256CoerceStringPrefaced(r, 16, "0x");
    assert (0 == strcmp ("0x0", s));
    free (s);
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
