//
//  BRCryptoAmount.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoAmount.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "BRCryptoAmount.h"

#include "support/BRInt.h"
#include "ethereum/util/BRUtilMath.h"

struct BRCryptoAmountRecord {
    BRCryptoUnit unit;
    BRCryptoBoolean isNegative;
    UInt256 value;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoAmount, cryptoAmount);

private_extern BRCryptoAmount
cryptoAmountCreateInternal (BRCryptoUnit unit,
                            BRCryptoBoolean isNegative,
                            UInt256 value,
                            int takeUnit) {
    BRCryptoAmount amount = malloc (sizeof (struct BRCryptoAmountRecord));

    amount->unit = takeUnit ? cryptoUnitTake (unit) : unit;
    amount->isNegative = isNegative;
    amount->value = value;
    amount->ref = CRYPTO_REF_ASSIGN (cryptoAmountRelease);

    return amount;
}

private_extern BRCryptoAmount
cryptoAmountCreate (BRCryptoUnit unit,
                    BRCryptoBoolean isNegative,
                    UInt256 value) {
    // Given a UInt256 -> conversion to unit already complete.
    return cryptoAmountCreateInternal (unit, isNegative, value, 1);
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
            : cryptoAmountCreateInternal (unit, isNegative, v, 1));
}

extern BRCryptoAmount
cryptoAmountCreateDouble (double value,
                          BRCryptoUnit unit) {
    int overflow;
    UInt256 valueAsUInt256 = createUInt256Double (value, cryptoUnitGetBaseDecimalOffset (unit), &overflow);

    return (overflow
            ? NULL
            : cryptoAmountCreateInternal (unit,
                                          (value < 0.0 ? CRYPTO_TRUE : CRYPTO_FALSE),
                                          valueAsUInt256,
                                          1));
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
    cryptoUnitGive (amount->unit);

    memset (amount, 0, sizeof(*amount));
    free (amount);
}

extern BRCryptoUnit
cryptoAmountGetUnit (BRCryptoAmount amount) {
    return cryptoUnitTake (amount->unit);
}

extern BRCryptoCurrency
cryptoAmountGetCurrency (BRCryptoAmount amount) {
    return cryptoUnitGetCurrency(amount->unit);
}

extern BRCryptoBoolean
cryptoAmountHasCurrency (BRCryptoAmount amount,
                         BRCryptoCurrency currency) {
    return cryptoUnitHasCurrency(amount->unit, currency);
}

extern BRCryptoBoolean
cryptoAmountIsNegative (BRCryptoAmount amount) {
    return amount->isNegative;
}

extern BRCryptoBoolean
cryptoAmountIsCompatible (BRCryptoAmount a1,
                          BRCryptoAmount a2) {
    return cryptoUnitIsCompatible (a1->unit, a2->unit);
}

extern BRCryptoBoolean
cryptoAmountIsZero (BRCryptoAmount amount) {
    return AS_CRYPTO_BOOLEAN (eqUInt256 (amount->value, UINT256_ZERO));
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
    assert (CRYPTO_TRUE == cryptoAmountIsCompatible (a1, a2));

    int overflow = 0;
    int negative = 0;

    if (CRYPTO_TRUE == a1->isNegative && CRYPTO_TRUE != a2->isNegative) {
        // (-x) + y = (y - x)
        UInt256 value = subUInt256_Negative (a2->value, a1->value, &negative);
        return cryptoAmountCreate (a1->unit, AS_CRYPTO_BOOLEAN(negative), value);
    }
    else if (CRYPTO_TRUE != a1->isNegative && CRYPTO_TRUE == a2->isNegative) {
        // x + (-y) = x - y
        UInt256 value = subUInt256_Negative (a1->value, a2->value, &negative);
        return cryptoAmountCreate(a1->unit, AS_CRYPTO_BOOLEAN(negative), value);
    }
    else if (CRYPTO_TRUE == a1->isNegative && CRYPTO_TRUE == a2->isNegative) {
        // (-x) + (-y) = - (x + y)
        UInt256 value = addUInt256_Overflow (a2->value, a1->value, &overflow);
        return overflow ? NULL :  cryptoAmountCreate (a1->unit, CRYPTO_TRUE, value);
    }
    else {
        UInt256 value = addUInt256_Overflow (a1->value, a2->value, &overflow);
        return overflow ? NULL :  cryptoAmountCreate (a1->unit, CRYPTO_FALSE, value);
    }
}

extern BRCryptoAmount
cryptoAmountSub (BRCryptoAmount a1,
                 BRCryptoAmount a2) {
    assert (CRYPTO_TRUE == cryptoAmountIsCompatible (a1, a2));

    int overflow = 0;
    int negative = 0;

    if (CRYPTO_TRUE == a1->isNegative && CRYPTO_TRUE != a2->isNegative) {
        // (-x) - y = - (x + y)
        UInt256 value = addUInt256_Overflow (a1->value, a2->value, &overflow);
        return overflow ? NULL : cryptoAmountCreate (a1->unit, CRYPTO_TRUE, value);
    }
    else if (CRYPTO_TRUE != a1->isNegative && CRYPTO_TRUE == a2->isNegative) {
        // x - (-y) = x + y
        UInt256 value = addUInt256_Overflow (a1->value, a2->value, &overflow);
        return overflow ? NULL : cryptoAmountCreate(a1->unit, CRYPTO_FALSE, value);
    }
    else if (CRYPTO_TRUE == a1->isNegative && CRYPTO_TRUE == a2->isNegative) {
        // (-x) - (-y) = y - x
        UInt256 value = subUInt256_Negative (a2->value, a1->value, &negative);
        return cryptoAmountCreate (a1->unit, AS_CRYPTO_BOOLEAN(negative), value);
    }
    else {
        UInt256 value = subUInt256_Negative (a1->value, a2->value, &negative);
        return cryptoAmountCreate (a1->unit, AS_CRYPTO_BOOLEAN(negative), value);
    }
}

extern BRCryptoAmount
cryptoAmountNegate (BRCryptoAmount amount) {
    return cryptoAmountCreate (amount->unit,
                               CRYPTO_TRUE == amount->isNegative ? CRYPTO_FALSE : CRYPTO_TRUE,
                               amount->value);
}

extern BRCryptoAmount
cryptoAmountConvertToUnit (BRCryptoAmount amount,
                           BRCryptoUnit unit) {
    return (CRYPTO_TRUE != cryptoUnitIsCompatible (amount->unit, unit)
            ? NULL
            : cryptoAmountCreate (unit, amount->isNegative, amount->value));
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

extern char *
cryptoAmountGetStringPrefaced (BRCryptoAmount amount,
                               int base,
                               const char *preface) {
    return coerceStringPrefaced (amount->value, base, preface);
}

extern UInt256
cryptoAmountGetValue (BRCryptoAmount amount) {
    return amount->value;
}
