/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.support.UInt256;
import com.breadwallet.crypto.CurrencyPair;
import com.breadwallet.corenative.crypto.BRCryptoComparison;
import com.breadwallet.corenative.crypto.CoreBRCryptoAmount;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.text.NumberFormat;
import java.util.Objects;

import static com.google.common.base.Preconditions.checkArgument;

/* package */
final class Amount implements com.breadwallet.crypto.Amount {

    /* package */
    static Optional<Amount> create(double value, com.breadwallet.crypto.Unit unit) {
        Unit unitImpl = Unit.from(unit);
        return CoreBRCryptoAmount.create(value, unitImpl.getCoreBRCryptoUnit()).transform((amount) -> new Amount(amount, unitImpl));
    }

    /* package */
    static Optional<Amount> create(long value, com.breadwallet.crypto.Unit unit) {
        Unit unitImpl = Unit.from(unit);
        return CoreBRCryptoAmount.create(value, unitImpl.getCoreBRCryptoUnit()).transform((amount) -> new Amount(amount, unitImpl));
    }

    /* package */
    static Optional<Amount> create(String value, boolean isNegative, com.breadwallet.crypto.Unit unit) {
        Unit unitImpl = Unit.from(unit);
        return CoreBRCryptoAmount.create(value, isNegative, unitImpl.getCoreBRCryptoUnit()).transform((amount) -> new Amount(amount, unitImpl));
    }

    /* package */
    static Amount createAsEth(UInt256.ByValue value, com.breadwallet.crypto.Unit unit) {
        Unit unitImpl = Unit.from(unit);
        return new Amount(CoreBRCryptoAmount.createAsEth(value, unitImpl.getCurrency().getCoreBRCryptoCurrency()), unitImpl);
    }

    /* package */
    static Amount createAsBtc(UnsignedLong value, com.breadwallet.crypto.Unit unit) {
        Unit unitImpl = Unit.from(unit);
        return new Amount(CoreBRCryptoAmount.createAsBtc(value, unitImpl.getCurrency().getCoreBRCryptoCurrency()), unitImpl);
    }

    /* package */
    static Amount create(CoreBRCryptoAmount core, Unit unit) {
        return new Amount(core, unit);
    }

    /* package */
    static Amount from(com.breadwallet.crypto.Amount amount) {
        if (amount instanceof Amount) {
            return (Amount) amount;
        }
        throw new IllegalArgumentException("Unsupported amount instance");
    }

    private static NumberFormat formatterWithUnit(com.breadwallet.crypto.Unit unit) {
        DecimalFormat formatter = (DecimalFormat) DecimalFormat.getCurrencyInstance().clone();
        DecimalFormatSymbols formatterSymbols = (DecimalFormatSymbols) formatter.getDecimalFormatSymbols().clone();

        String symbol = unit.getSymbol();
        formatterSymbols.setInternationalCurrencySymbol(symbol);
        formatterSymbols.setCurrencySymbol(symbol);

        formatter.setParseBigDecimal(true);
        formatter.setRoundingMode(RoundingMode.HALF_UP);
        formatter.setDecimalFormatSymbols(formatterSymbols);
        formatter.setMaximumFractionDigits(unit.getDecimals().intValue());

        return formatter;
    }

    private final CoreBRCryptoAmount core;
    private final Unit unit;

    private Amount(CoreBRCryptoAmount core, Unit unit) {
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
    public boolean hasCurrency(com.breadwallet.crypto.Currency currency) {
        return core.hasCurrency(Currency.from(currency).getCoreBRCryptoCurrency());
    }

    @Override
    public boolean isCompatible(com.breadwallet.crypto.Amount withAmount) {
        return core.isCompatible(from(withAmount).core);
    }

    @Override
    public boolean isNegative() {
        return core.isNegative();
    }

    @Override
    public Optional<Amount> add(com.breadwallet.crypto.Amount o) {
        checkArgument(isCompatible(o));

        return core.add(from(o).core).transform(a -> new Amount(a, unit));
    }

    @Override
    public Optional<Amount> sub(com.breadwallet.crypto.Amount o) {
        checkArgument(isCompatible(o));

        return core.sub(from(o).core).transform(a -> new Amount(a, unit));
    }

    @Override
    public Amount negate() {
        return new Amount(core.negate(), unit);
    }

    @Override
    public Optional<String> toStringAsUnit(com.breadwallet.crypto.Unit asUnit) {
        return toStringAsUnit(asUnit, null);
    }

    @Override
    public Optional<String> toStringAsUnit(com.breadwallet.crypto.Unit asUnit, @Nullable NumberFormat numberFormatter) {
        numberFormatter = numberFormatter != null ? numberFormatter : formatterWithUnit(asUnit);
        return doubleAmount(asUnit).transform(numberFormatter::format);
    }

    @Override
    public Optional<String> toStringFromPair(CurrencyPair pair) {
        return toStringFromPair(pair, null);
    }

    @Override
    public Optional<String> toStringFromPair(CurrencyPair pair, @Nullable NumberFormat numberFormatter) {
        Optional<? extends com.breadwallet.crypto.Amount> amount = pair.exchangeAsBase(this);
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
    public int compareTo(com.breadwallet.crypto.Amount o) {
        switch (core.compare(from(o).core)) {
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

    @Override
    public Optional<Double> doubleAmount(com.breadwallet.crypto.Unit asUnit) {
        return core.getDouble(Unit.from(asUnit).getCoreBRCryptoUnit());
    }

    /* package */
    CoreBRCryptoAmount getCoreBRCryptoAmount() {
        return core;
    }
}
