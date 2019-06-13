//
//  BRCryptoAmount.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
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

#include "BRCryptoAmount.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "support/BRInt.h"
#include "ethereum/util/BRUtilMath.h"

static void
cryptoAmountRelease (BRCryptoAmount amount);

struct BRCryptoAmountRecord {
    BRCryptoCurrency currency;
    BRCryptoBoolean isNegative;
    UInt256 value;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoAmount, cryptoAmount);

private_extern BRCryptoAmount
cryptoAmountCreateInternal (BRCryptoCurrency currency,
                            BRCryptoBoolean isNegative,
                            UInt256 value,
                            int takeCurrency) {
    BRCryptoAmount amount = malloc (sizeof (struct BRCryptoAmountRecord));

    amount->currency = takeCurrency ? cryptoCurrencyTake (currency) : currency;
    amount->isNegative = isNegative;
    amount->value = value;
    amount->ref = CRYPTO_REF_ASSIGN (cryptoAmountRelease);

    return amount;
}

private_extern BRCryptoAmount
cryptoAmountCreate (BRCryptoCurrency currency,
                    BRCryptoBoolean isNegative,
                    UInt256 value) {
    return cryptoAmountCreateInternal (currency, isNegative, value, 1);
}

static BRCryptoAmount
cryptoAmountCreateUInt256 (UInt256 v,
                           BRCryptoBoolean isNegative,
                           BRCryptoUnit unit) {
    int powOverflow = 0, mulOverflow = 0;

    uint8_t decimals = cryptoUnitGetBaseDecimalOffset (unit);

    if (0 != decimals)
        v = mulUInt256_Overflow (v, createUInt256Power(decimals, &powOverflow), &mulOverflow);

    return (powOverflow || mulOverflow ? NULL
            : cryptoAmountCreateInternal (cryptoUnitGetCurrency (unit), isNegative, v, 0));
}

extern BRCryptoAmount
cryptoAmountCreateDouble (double value,
                          BRCryptoUnit unit) {
    uint8_t decimals = cryptoUnitGetBaseDecimalOffset (unit);
    long double v = fabs(value) * powl (10.0, decimals);

    if (v > INT64_MAX) return NULL;

    return cryptoAmountCreateInternal (cryptoUnitGetCurrency(unit),
                                       (value < 0.0 ? CRYPTO_TRUE : CRYPTO_FALSE),
                                       createUInt256((uint64_t) v),
                                       0);
}

extern BRCryptoAmount
cryptoAmountCreateInteger (int64_t value,
                           BRCryptoUnit unit) {

    UInt256 v = createUInt256 (value < 0 ? -value : value);
    return cryptoAmountCreateUInt256 (v, (value < 0 ? CRYPTO_TRUE : CRYPTO_FALSE), unit);
}

extern BRCryptoAmount
cryptoAmountCreateString (const char *value,
                          BRCryptoBoolean isNegative,
                          BRCryptoUnit unit) {
    UInt256 v;
    BRCoreParseStatus status;

    // Try to parse as an integer
    v = createUInt256Parse (value, 0, &status);

    // if that fails, try to parse as a decimal
    if (CORE_PARSE_OK != status) {
        v = createUInt256ParseDecimal (value, cryptoUnitGetBaseDecimalOffset (unit), &status);
        unit = cryptoUnitGetBaseUnit(unit);
        cryptoUnitGive(unit);
    }

    return (CORE_PARSE_OK != status ? NULL : cryptoAmountCreateUInt256 (v, isNegative, unit));
}

static void
cryptoAmountRelease (BRCryptoAmount amount) {
//    printf ("Amount: Release\n");
    cryptoCurrencyGive (amount->currency);
    free (amount);
}

extern BRCryptoCurrency
cryptoAmountGetCurrency (BRCryptoAmount amount) {
    return cryptoCurrencyTake (amount->currency);
}

extern BRCryptoBoolean
cryptoAmountHasCurrency (BRCryptoAmount amount,
                         BRCryptoCurrency currency) {
    return AS_CRYPTO_BOOLEAN (amount->currency == currency);
}

extern BRCryptoBoolean
cryptoAmountIsNegative (BRCryptoAmount amount) {
    return amount->isNegative;
}

extern BRCryptoBoolean
cryptoAmountIsCompatible (BRCryptoAmount a1,
                          BRCryptoAmount a2) {
    return cryptoCurrencyIsIdentical (a1->currency, a2->currency);
}

static BRCryptoComparison
cryptoCompareUInt256 (UInt256 v1, UInt256 v2) {
    switch (compareUInt256 (v1, v2)) {
        case -1: return CRYPTO_COMPARE_LT;
        case  0: return CRYPTO_COMPARE_EQ;
        case +1: return CRYPTO_COMPARE_GT;
        default: assert (0);
    }
}

extern BRCryptoComparison
cryptoAmountCompare (BRCryptoAmount a1,
                     BRCryptoAmount a2) {
    assert (CRYPTO_TRUE == cryptoAmountIsCompatible(a1, a2));

    if (CRYPTO_TRUE == a1->isNegative && CRYPTO_TRUE != a2->isNegative)
        return CRYPTO_COMPARE_LT;
    else if (CRYPTO_TRUE != a1->isNegative && CRYPTO_TRUE == a2->isNegative)
        return CRYPTO_COMPARE_GT;
    else if (CRYPTO_TRUE == a1->isNegative && CRYPTO_TRUE == a2->isNegative)
        // both negative -> swap comparison
        return cryptoCompareUInt256 (a2->value, a1->value);
    else
        // both positive -> same comparison
        return cryptoCompareUInt256 (a1->value, a2->value);
}

extern BRCryptoAmount
cryptoAmountAdd (BRCryptoAmount a1,
                 BRCryptoAmount a2) {
    assert (CRYPTO_TRUE == cryptoAmountIsCompatible(a1, a2));

    int overflow = 0;
    UInt256 sum = addUInt256_Overflow(a1->value, a2->value, &overflow);

    return overflow ? NULL : cryptoAmountCreate (a1->currency, CRYPTO_FALSE, sum);
}

extern BRCryptoAmount
cryptoAmountSub (BRCryptoAmount a1,
                 BRCryptoAmount a2) {
    int overflow = 0;
    int negative = 0;

    if (CRYPTO_TRUE == a1->isNegative && CRYPTO_TRUE != a2->isNegative) {
        // (-x) - y = - (x + y)
        UInt256 value = addUInt256_Overflow (a1->value, a2->value, &overflow);
        return overflow ? NULL : cryptoAmountCreate (a1->currency, CRYPTO_TRUE, value);
    }
    else if (CRYPTO_TRUE != a1->isNegative && CRYPTO_TRUE == a2->isNegative) {
        // x - (-y) = x + y
        UInt256 value = addUInt256_Overflow (a1->value, a2->value, &overflow);
        return overflow ? NULL : cryptoAmountCreate(a1->currency, CRYPTO_FALSE, value);
    }
    else if (CRYPTO_TRUE == a1->isNegative && CRYPTO_TRUE == a2->isNegative) {
        // (-x) - (-y) = y - x
        UInt256 value = subUInt256_Negative (a2->value, a1->value, &negative);
        return cryptoAmountCreate (a1->currency, AS_CRYPTO_BOOLEAN(negative), value);
    }
    else {
        UInt256 value = subUInt256_Negative (a1->value, a2->value, &negative);
        return cryptoAmountCreate (a1->currency, AS_CRYPTO_BOOLEAN(negative), value);
    }
}

extern BRCryptoAmount
cryptoAmountNegate (BRCryptoAmount amount) {
    return cryptoAmountCreate (amount->currency,
                               CRYPTO_TRUE == amount->isNegative ? CRYPTO_FALSE : CRYPTO_TRUE,
                               amount->value);
}

extern double
cryptoAmountGetDouble (BRCryptoAmount amount,
                       BRCryptoUnit unit,
                       BRCryptoBoolean *overflow) {
    assert (NULL != overflow);
    long double power  = powl (10.0, cryptoUnitGetBaseDecimalOffset(unit));
    long double result = coerceDouble (amount->value, (int*) overflow) / power;
    return (CRYPTO_TRUE == amount->isNegative ? -result : result);
}

extern uint64_t
cryptoAmountGetIntegerRaw (BRCryptoAmount amount,
                           BRCryptoBoolean *overflow) {
    *overflow = (0 != amount->value.u64 [3] ||
                 0 != amount->value.u64 [2] ||
                 0 != amount->value.u64 [1]);
    return *overflow ? 0 : amount->value.u64[0];
}

extern UInt256
cryptoAmountGetValue (BRCryptoAmount amount) {
    return amount->value;
}
