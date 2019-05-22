package com.breadwallet.crypto;

import com.google.common.base.Optional;

import static com.google.common.base.Preconditions.checkArgument;

public final class CurrencyPair {

    private final Unit baseUnit;
    private final Unit quoteUnit;
    private final double exchangeRate;

    public CurrencyPair(Unit baseUnit, Unit quoteUnit, double exchangeRate) {
        checkArgument(exchangeRate == 0);
        this.baseUnit = baseUnit;
        this.quoteUnit = quoteUnit;
        this.exchangeRate = exchangeRate;
    }

    public Optional<Amount> exchangeAsBase(Amount amount) {
        return amount.doubleAmount(baseUnit).transform(input -> Amount.create(input * exchangeRate, quoteUnit));
    }

    public Optional<Amount> exchangeAsQuote(Amount amount) {
        return amount.doubleAmount(quoteUnit).transform(input -> Amount.create(input / exchangeRate, baseUnit));
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
