/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.CurrencyPair;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoBoolean;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoComparison;
import com.breadwallet.crypto.libcrypto.crypto.CoreBRCryptoAmount;
import com.google.common.base.Optional;
import com.sun.jna.ptr.IntByReference;

import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.text.NumberFormat;
import java.util.Objects;

import static com.google.common.base.Preconditions.checkArgument;
import static com.google.common.base.Preconditions.checkState;

/* package */
final class AmountImpl implements Amount {

    /* package */
    static Optional<Amount> create(double value, Unit unit) {
        UnitImpl unitImpl = UnitImpl.from(unit);
        return CoreBRCryptoAmount.create(value, unitImpl.getCoreBRCryptoUnit()).transform((amount) -> new AmountImpl(amount, unitImpl));
    }

    /* package */
    static Optional<Amount> create(long value, Unit unit) {
        UnitImpl unitImpl = UnitImpl.from(unit);
        return CoreBRCryptoAmount.create(value, unitImpl.getCoreBRCryptoUnit()).transform((amount) -> new AmountImpl(amount, unitImpl));
    }

    /* package */
    static Optional<Amount> create(String value, boolean isNegative, Unit unit) {
        UnitImpl unitImpl = UnitImpl.from(unit);
        return CoreBRCryptoAmount.create(value, isNegative, unitImpl.getCoreBRCryptoUnit()).transform((amount) -> new AmountImpl(amount, unitImpl));
    }

    /* package */
    static AmountImpl createAsBtc(long value, Unit unit) {
        UnitImpl unitImpl = UnitImpl.from(unit);
        return new AmountImpl(CoreBRCryptoAmount.createAsBtc(value, unitImpl.getCurrency().getCoreBRCryptoCurrency()), unitImpl);
    }

    /* package */
    static AmountImpl from(Amount amount) {
        if (amount instanceof AmountImpl) {
            return (AmountImpl) amount;
        }
        throw new IllegalArgumentException("Unsupported amount instance");
    }

    private static NumberFormat formatterWithUnit(Unit unit) {
        DecimalFormat formatter = (DecimalFormat) DecimalFormat.getCurrencyInstance().clone();
        DecimalFormatSymbols formatterSymbols = (DecimalFormatSymbols) formatter.getDecimalFormatSymbols().clone();

        String symbol = unit.getSymbol();
        formatterSymbols.setInternationalCurrencySymbol(symbol);
        formatterSymbols.setCurrencySymbol(symbol);

        formatter.setParseBigDecimal(true);
        formatter.setRoundingMode(RoundingMode.HALF_UP);
        formatter.setDecimalFormatSymbols(formatterSymbols);
        formatter.setMaximumFractionDigits(unit.getDecimals());

        return formatter;
    }

    private final CoreBRCryptoAmount core;
    private final UnitImpl unit;

    private AmountImpl(CoreBRCryptoAmount core, UnitImpl unit) {
        this.core = core;
        this.unit = unit;
    }

    @Override
    public Currency getCurrency() {
        return unit.getCurrency();
    }

    @Override
    public Unit getUnit() {
        return unit;
    }

    @Override
    public boolean hasCurrency(Currency currency) {
        CurrencyImpl currencyImpl = CurrencyImpl.from(currency);
        return currencyImpl.getCoreBRCryptoCurrency().equals(core.getCurrency());
    }

    @Override
    public boolean isCompatible(Amount withAmount) {
        AmountImpl amountImpl = from(withAmount);
        return core.isCompatible(amountImpl.core);
    }

    @Override
    public boolean isNegative() {
        return core.isNegative();
    }

    @Override
    public Optional<Amount> add(Amount o) {
        checkArgument(isCompatible(o));

        AmountImpl amountImpl = from(o);
        CoreBRCryptoAmount amount = core.add(amountImpl.core);
        if (amount == null) {
            return Optional.absent();
        }

        return Optional.of(new AmountImpl(amount, unit));
    }

    @Override
    public Optional<Amount> sub(Amount o) {
        checkArgument(isCompatible(o));

        AmountImpl amountImpl = from(o);
        CoreBRCryptoAmount amount = core.sub(amountImpl.core);
        if (amount == null) {
            return Optional.absent();
        }

        return Optional.of(new AmountImpl(amount, unit));
    }

    @Override
    public AmountImpl negate() {
        return new AmountImpl(core.negate(), unit);
    }

    @Override
    public Optional<String> toStringAsUnit(Unit asUnit) {
        return toStringAsUnit(asUnit, null);
    }

    @Override
    public Optional<String> toStringAsUnit(Unit asUnit, @Nullable NumberFormat numberFormatter) {
        numberFormatter = numberFormatter != null ? numberFormatter : formatterWithUnit(asUnit);
        return doubleAmount(asUnit).transform(numberFormatter::format);
    }

    @Override
    public Optional<String> toStringFromPair(CurrencyPair pair) {
        return toStringFromPair(pair, null);
    }

    @Override
    public Optional<String> toStringFromPair(CurrencyPair pair, @Nullable NumberFormat numberFormatter) {
        Optional<Amount> amount = pair.exchangeAsBase(this);
        if (amount.isPresent()) {
            return amount.get().toStringAsUnit(pair.getQuoteUnit(), numberFormatter);
        } else {
            return Optional.absent();
        }
    }

    @Override
    public String toStringWithBase(int base, String preface) {
        return core.toStringWithBase(base, preface);
    }

    @Override
    public String toString() {
        // TODO(discuss): Do we want to return a non-localized value here? Is there a localization of NaN?
        return toStringAsUnit(unit).or("<nan>");
    }

    @Override
    public int compareTo(Amount o) {
        AmountImpl amountImpl = from(o);
        switch (core.compare(amountImpl.core)) {
            case BRCryptoComparison.CRYPTO_COMPARE_EQ:
                return 0;
            case BRCryptoComparison.CRYPTO_COMPARE_LT:
                return -1;
            case BRCryptoComparison.CRYPTO_COMPARE_GT:
                return 1;
            default:
                throw new IllegalStateException("Invalid amount comparison");
        }
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }

        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        AmountImpl amount = (AmountImpl) o;
        return BRCryptoComparison.CRYPTO_COMPARE_EQ == core.compare(amount.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core, unit);
    }

    @Override
    public Optional<Double> doubleAmount(Unit asUnit) {
        UnitImpl unitImpl = UnitImpl.from(asUnit);
        IntByReference overflowRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        double value = core.getDouble(unitImpl.getCoreBRCryptoUnit(), overflowRef);
        return overflowRef.getValue() == BRCryptoBoolean.CRYPTO_TRUE ? Optional.absent() : Optional.of(value);
    }

    /* package */
    long integerRawAmount() {
        // TODO(discuss): doubleAmount returns an optional based on overflow; shouldn't this?
        IntByReference overflowRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        long value = core.getIntegerRaw(overflowRef);
        checkState(BRCryptoBoolean.CRYPTO_FALSE == overflowRef.getValue());
        return value;
    }
}
