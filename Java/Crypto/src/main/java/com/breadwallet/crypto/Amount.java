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

import android.support.annotation.Nullable;

import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoBoolean;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoComparison;
import com.breadwallet.crypto.jni.crypto.CoreBRCryptoAmount;
import com.google.common.base.Optional;
import com.sun.jna.ptr.IntByReference;

import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.text.NumberFormat;
import java.util.Objects;

import static com.google.common.base.Preconditions.checkArgument;
import static com.google.common.base.Preconditions.checkState;

// TODO: Swift doesn't acknowledge that the creates can fail
public final class Amount implements Comparable<Amount> {

    /* package */
    static Amount createAsBtc(long value, Unit unit) {
        return new Amount(CoreBRCryptoAmount.createAsBtc(value, unit.getCurrency().core), unit);
    }

    public static Optional<Amount> create(double value, Unit unit) {
        return CoreBRCryptoAmount.create(value, unit.core).transform((amount) -> new Amount(amount, unit));
    }

    public static Optional<Amount> create(long value, Unit unit) {
        return CoreBRCryptoAmount.create(value, unit.core).transform((amount) -> new Amount(amount, unit));
    }

    public static Optional<Amount> create(String value, boolean isNegative, Unit unit) {
        return CoreBRCryptoAmount.create(value, isNegative, unit.core).transform((amount) -> new Amount(amount, unit));
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
    private final Unit unit;

    private Amount(CoreBRCryptoAmount core, Unit unit) {
        this.core = core;
        this.unit = unit;
    }

    public Currency getCurrency() {
        return unit.getCurrency();
    }

    public Unit getUnit() {
        return unit;
    }

    public boolean hasCurrency(Currency currency) {
        return currency.core.equals(core.getCurrency());
    }

    public boolean isCompatible(Amount withAmount) {
        return BRCryptoBoolean.CRYPTO_TRUE == core.isCompatible(withAmount.core);
    }

    public boolean isNegative() {
        return BRCryptoBoolean.CRYPTO_TRUE == core.isNegative();
    }

    public Optional<Amount> add(Amount o) {
        checkArgument(isCompatible(o));

        CoreBRCryptoAmount amount = core.add(o.core);
        if (amount == null) {
            return Optional.absent();
        }

        return Optional.of(new Amount(amount, unit));
    }

    public Optional<Amount> sub(Amount o) {
        checkArgument(isCompatible(o));

        CoreBRCryptoAmount amount = core.sub(o.core);
        if (amount == null) {
            return Optional.absent();
        }

        return Optional.of(new Amount(amount, unit));
    }

    public Amount negate() {
        return new Amount(core.negate(), unit);
    }

    /* package */ Optional<Double> doubleAmount(Unit asUnit) {
        IntByReference overflowRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        double value = core.getDouble(asUnit.core, overflowRef);
        return overflowRef.getValue() == BRCryptoBoolean.CRYPTO_TRUE ? Optional.absent() : Optional.of(value);
    }

    public Optional<String> toStringAsUnit(Unit asUnit, @Nullable NumberFormat numberFormatter) {
        numberFormatter = numberFormatter != null ? numberFormatter : formatterWithUnit(asUnit);
        return doubleAmount(asUnit).transform(numberFormatter::format);
    }

    public Optional<String> toStringFromPair(CurrencyPair pair, @Nullable NumberFormat numberFormatter) {
        Optional<Amount> amount = pair.exchangeAsBase(this);
        if (amount.isPresent()) {
            return amount.get().toStringAsUnit(pair.getQuoteUnit(), numberFormatter);
        } else {
            return Optional.absent();
        }
    }

    public String toStringWithBase(int base, String preface) {
        return core.toStringWithBase(base, preface);
    }

    @Override
    public int compareTo(Amount o) {
        switch (core.compare(o.core)) {
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

        Amount amount = (Amount) o;
        return BRCryptoComparison.CRYPTO_COMPARE_EQ == core.compare(amount.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core, unit);
    }

    /* package */
    long asBtc() {
        IntByReference overflowRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        long value = core.getIntegerRaw(overflowRef);
        checkState(BRCryptoBoolean.CRYPTO_FALSE == overflowRef.getValue());
        return value;
    }
}
