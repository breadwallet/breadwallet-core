/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoAmount;
import com.breadwallet.crypto.CurrencyPair;
import com.breadwallet.corenative.crypto.BRCryptoComparison;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.text.NumberFormat;
import java.util.Objects;

import static com.google.common.base.Preconditions.checkArgument;

/* package */
final class Amount implements com.breadwallet.crypto.Amount {

    /* package */
    static Amount create(double value, com.breadwallet.crypto.Unit unit) {
        Unit cryptoUnit = Unit.from(unit);
        BRCryptoAmount core = BRCryptoAmount.create(value, cryptoUnit.getCoreBRCryptoUnit());
        return Amount.create(core);
    }

    /* package */
    static Amount create(long value, com.breadwallet.crypto.Unit unit) {
        Unit cryptoUnit = Unit.from(unit);
        BRCryptoAmount core = BRCryptoAmount.create(value, cryptoUnit.getCoreBRCryptoUnit());
        return Amount.create(core);
    }

    /* package */
    static Optional<Amount> create(String value, boolean isNegative, com.breadwallet.crypto.Unit unit) {
        Unit cryptoUnit = Unit.from(unit);
        Optional<BRCryptoAmount> core = BRCryptoAmount.create(value, isNegative, cryptoUnit.getCoreBRCryptoUnit());
        return core.transform(Amount::create);
    }

    /* package */
    static Amount create(BRCryptoAmount core) {
        Amount amount = new Amount(core);
        ReferenceCleaner.register(amount, core::give);
        return amount;
    }

    /* package */
    static Amount from(com.breadwallet.crypto.Amount amount) {
        if (amount == null) {
            return null;
        }

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

        int decimals = unit.getDecimals().intValue();
        formatter.setParseBigDecimal(0 != decimals);
        formatter.setRoundingMode(RoundingMode.HALF_EVEN);
        formatter.setDecimalFormatSymbols(formatterSymbols);
        formatter.setMaximumIntegerDigits(Integer.MAX_VALUE);
        formatter.setMaximumFractionDigits(decimals);

        return formatter;
    }

    private final BRCryptoAmount core;

    private final Supplier<Unit> unitSupplier;
    private final Supplier<Currency> currencySupplier;
    private final Supplier<String> toStringSupplier;

    private Amount(BRCryptoAmount core) {
        this.core = core;

        this.currencySupplier = Suppliers.memoize(() -> Currency.create(core.getCurrency()));
        this.unitSupplier = Suppliers.memoize(() -> Unit.create(core.getUnit()));
        this.toStringSupplier = Suppliers.memoize(() -> toStringAsUnit(getUnit()).or("<nan>"));
    }

    @Override
    public Currency getCurrency() {
        return currencySupplier.get();
    }

    @Override
    public Unit getUnit() {
        return unitSupplier.get();
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
    public boolean isZero() {
        return core.isZero();
    }

    @Override
    public Optional<Amount> add(com.breadwallet.crypto.Amount o) {
        checkArgument(isCompatible(o));

        return core.add(from(o).core).transform(Amount::create);
    }

    @Override
    public Optional<Amount> sub(com.breadwallet.crypto.Amount o) {
        checkArgument(isCompatible(o));

        return core.sub(from(o).core).transform(Amount::create);
    }

    @Override
    public Amount negate() {
        return Amount.create(core.negate());
    }

    @Override
    public Optional<Amount> convert(com.breadwallet.crypto.Unit toUnit) {
        return core.convert(Unit.from(toUnit).getCoreBRCryptoUnit()).transform(Amount::create);
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
        return toStringSupplier.get();
    }

    @Override
    public int compareTo(com.breadwallet.crypto.Amount o) {
        switch (core.compare(from(o).core)) {
            case CRYPTO_COMPARE_EQ: return 0;
            case CRYPTO_COMPARE_LT: return -1;
            case CRYPTO_COMPARE_GT: return 1;
            default: throw new IllegalStateException("Invalid amount comparison");
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
        return Objects.hash(toString());
    }

    @Override
    public Optional<Double> doubleAmount(com.breadwallet.crypto.Unit asUnit) {
        return core.getDouble(Unit.from(asUnit).getCoreBRCryptoUnit());
    }

    /* package */
    BRCryptoAmount getCoreBRCryptoAmount() {
        return core;
    }
}
