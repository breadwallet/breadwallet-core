/*
 * Amount
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;

import java.text.NumberFormat;

public interface Amount extends Comparable<Amount> {

    static Optional<Amount> create(double value, Unit unit) {
        return CryptoApi.getProvider().amountProvider().create(value, unit);
    }

    static Optional<Amount> create(long value, Unit unit) {
        return CryptoApi.getProvider().amountProvider().create(value, unit);
    }

    static Optional<Amount> create(String value, boolean isNegative, Unit unit) {
        return CryptoApi.getProvider().amountProvider().create(value, isNegative, unit);
    }

    Currency getCurrency();

    Unit getUnit();

    boolean hasCurrency(Currency currency);

    boolean isCompatible(Amount withAmount);

    boolean isNegative();

    Optional<Amount> add(Amount o);

    Optional<Amount> sub(Amount o);

    Amount negate();

    Optional<String> toStringAsUnit(Unit asUnit);

    Optional<String> toStringAsUnit(Unit asUnit, NumberFormat numberFormatter);

    Optional<String> toStringFromPair(CurrencyPair pair);

    Optional<String> toStringFromPair(CurrencyPair pair, NumberFormat numberFormatter);

    String toStringWithBase(int base, String preface);

    String toString();

    boolean equals(Object o);

    int hashCode();

    Optional<Double> doubleAmount(Unit asUnit);
}
