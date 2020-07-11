//
//  BRCryptoAmount.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoAmount_h
#define BRCryptoAmount_h

#include "BRCryptoBase.h"
#include "BRCryptoCurrency.h"
#include "BRCryptoUnit.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CRYPTO_COMPARE_LT,
        CRYPTO_COMPARE_EQ,
        CRYPTO_COMPARE_GT
    } BRCryptoComparison;

    typedef struct BRCryptoAmountRecord *BRCryptoAmount;

    /**
     * Create an amount from `value` in `unit`.  If the amount is "2 Bitcoin" then, for example,
     * the amount can be created from either of {2.0, BTC} or {2e8, SAT}.
     *
     * @note The internal representation is always as a UInt256 in the currency's baseUnit (aka
     * the 'integer unit').  This allows easy arithmetic operations w/o conversions.  Conversions
     * to the base unit happens on creation, and conversion from the base unit happens on display.
     *
     * @param value
     * @param unit
     *
     * @return An amount
     */
    extern BRCryptoAmount
    cryptoAmountCreateDouble (double value,
                              BRCryptoUnit unit);

    /**
     * Create an amount form `value` in `unit`.  See discusion on `cryptoAmountCreateDouble()`
     *
     * @param value
     * @param unit
     *
     *@return An Amount
     */
    extern BRCryptoAmount
    cryptoAmountCreateInteger (int64_t value,
                               BRCryptoUnit unit);

    /**
     * Create an amount from `value` and `unit`. See discusion on `cryptoAmountCreateDouble()`
     *
     * @note there are some constraints on `const char *value` that need to be described.
     *
     * @param value
     * @param isNegative
     * @param unit
     *
     * @return An amount
     */
    extern BRCryptoAmount
    cryptoAmountCreateString (const char *value,
                              BRCryptoBoolean isNegative,
                              BRCryptoUnit unit);

    /**
     * Returns the amount's unit.
     *
     * @param amount The amount
     *
     * @return The amount's unit, typically use for default display w/ an incremented reference
     *    count (aka 'taken')
     */
    extern BRCryptoUnit
    cryptoAmountGetUnit (BRCryptoAmount amount);

    /**
     * Returns the amount's currency
     *
     * @param amount The amount
     *
     * @return The currency w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoCurrency
    cryptoAmountGetCurrency (BRCryptoAmount amount);

    extern BRCryptoBoolean
    cryptoAmountHasCurrency (BRCryptoAmount amount,
                             BRCryptoCurrency currency);
    
    extern BRCryptoBoolean
    cryptoAmountIsNegative (BRCryptoAmount amount);


    /**
     * Check of two amount's are compatible; they are compatible if they have the same currency and
     * thus can be added.
     *
     * @param a1
     * @param a2
     *
     * @return
     */
    extern BRCryptoBoolean
    cryptoAmountIsCompatible (BRCryptoAmount a1,
                              BRCryptoAmount a2);

    extern BRCryptoBoolean
    cryptoAmountIsZero (BRCryptoAmount amount);

    extern BRCryptoComparison
    cryptoAmountCompare (BRCryptoAmount a1,
                         BRCryptoAmount a2);

    extern BRCryptoAmount
    cryptoAmountAdd (BRCryptoAmount a1,
                     BRCryptoAmount a2);

    extern BRCryptoAmount
    cryptoAmountSub (BRCryptoAmount a1,
                     BRCryptoAmount a2);

    extern BRCryptoAmount
    cryptoAmountNegate (BRCryptoAmount amount);

    /**
     * Convert `amount` into `unit`
     *
     * @param amount
     * @param unit
     *
     * @note: if `unit` is incompatible, then NULL is returned.
     *
     * @return An amount
     */
    extern BRCryptoAmount
    cryptoAmountConvertToUnit (BRCryptoAmount amount,
                               BRCryptoUnit unit);

    /**
     * Return `amount` as a double in `unit`.  For example, if amount is "2 BTC" and unit is
     * SAT then the returned value will be 2e8; if amount is "2e6 SAT" and unit is "BTC" then the
     * return value will be 0.02.
     *
     * @param amount
     * @param unit
     * @param overflow
     *
     * @return A double
     */
    extern double
    cryptoAmountGetDouble (BRCryptoAmount amount,
                           BRCryptoUnit unit,
                           BRCryptoBoolean *overflow);

    /**
     * Return `amount` as a UInt64 if the representation of `amount` in the base unit is less than
     * or equal to UINT64_MAX; otherwise set overflow.
     *
     * @param amount
     * @param overflow
     *
     * @return
     */
    extern uint64_t
    cryptoAmountGetIntegerRaw (BRCryptoAmount amount,
                               BRCryptoBoolean *overflow);

    extern char *
    cryptoAmountGetStringPrefaced (BRCryptoAmount amount,
                                   int base,
                                   const char *preface);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoAmount, cryptoAmount);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoAmount_h */
