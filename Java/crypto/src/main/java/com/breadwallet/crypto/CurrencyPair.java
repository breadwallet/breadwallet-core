/*
 * CurrencyPair
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;

import static com.google.common.base.Preconditions.checkArgument;

public final class CurrencyPair {

    private final Unit baseUnit;
    private final Unit quoteUnit;
    private final double exchangeRate;

    public CurrencyPair(Unit baseUnit, Unit quoteUnit, double exchangeRate) {
        checkArgument(exchangeRate != 0);
        this.baseUnit = baseUnit;
        this.quoteUnit = quoteUnit;
        this.exchangeRate = exchangeRate;
    }

    public Optional<Amount> exchangeAsBase(Amount amount) {
        Optional<Double> doubleAmount = amount.doubleAmount(baseUnit);
        return doubleAmount.transform(a -> Amount.create(doubleAmount.get() * exchangeRate, quoteUnit));
    }

    public Optional<Amount> exchangeAsQuote(Amount amount) {
        Optional<Double> doubleAmount = amount.doubleAmount(quoteUnit);
        return doubleAmount.transform(a -> Amount.create(doubleAmount.get() / exchangeRate, baseUnit));
    }

    public Unit getBaseUnit() {
        return baseUnit;
    }

    public Unit getQuoteUnit() {
        return quoteUnit;
    }

    public double getExchangeRate() {
        return exchangeRate;
    }

    @Override
    public String toString() {
        return String.format("%s/%s=%s", baseUnit.getName(), quoteUnit.getName(), exchangeRate);
    }
}
