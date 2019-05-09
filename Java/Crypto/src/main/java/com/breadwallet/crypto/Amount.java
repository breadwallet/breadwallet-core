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

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoAmount;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoBoolean;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoComparison;
import com.google.common.base.Optional;
import com.sun.jna.ptr.IntByReference;

import java.util.Objects;

import static com.google.common.base.Preconditions.checkArgument;

public final class Amount implements Comparable<Amount> {

    /* package */ final BRCryptoAmount core;
    private final Unit unit;

    public static Amount create(double value, Unit unit) {
        return new Amount(CryptoLibrary.INSTANCE.cryptoAmountCreateDouble(value, unit.core), unit);
    }

    public static Amount create(long value, Unit unit) {
        return new Amount(CryptoLibrary.INSTANCE.cryptoAmountCreateInteger(value, unit.core), unit);
    }

    public static Optional<Amount> create(String value, boolean isNegative, Unit unit) {
        int isNegativeEnum = isNegative ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE;
        BRCryptoAmount amount = CryptoLibrary.INSTANCE.cryptoAmountCreateString(value, isNegativeEnum, unit.core);
        if (amount == null) {
            return Optional.absent();
        }
        return Optional.of(new Amount(amount, unit));
    }

    private Amount(BRCryptoAmount core, Unit unit) {
        this.core = core;
        this.unit = unit;
    }

    @Override
    protected void finalize() {
        CryptoLibrary.INSTANCE.cryptoAmountGive(core);
    }

    public Currency getCurrency() {
        return unit.getCurrency();
    }

    public boolean hasCurrency(Currency currency) {
        return currency.core.equals(CryptoLibrary.INSTANCE.cryptoAmountGetCurrency(core));
    }

    public boolean isCompatible(Amount withAmount) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAmountIsCompatible(core, withAmount.core);
    }

    public boolean isNegative() {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAmountIsNegative(core);
    }

    public Optional<Amount> add(Amount o) {
        checkArgument(isCompatible(o));

        BRCryptoAmount amount = CryptoLibrary.INSTANCE.cryptoAmountAdd(core, o.core);
        if (amount == null) {
            return Optional.absent();
        }

        return Optional.of(new Amount(amount, unit));
    }

    public Optional<Amount> sub(Amount o) {
        checkArgument(isCompatible(o));

        BRCryptoAmount amount = CryptoLibrary.INSTANCE.cryptoAmountSub(core, o.core);
        if (amount == null) {
            return Optional.absent();
        }

        return Optional.of(new Amount(amount, unit));
    }

    public Amount negate() {
        return new Amount(CryptoLibrary.INSTANCE.cryptoAmountNegate(core), unit);
    }

    // TODO: String/Formatting functions

    /* package */ Optional<Double> doubleAmount(Unit asUnit) {
        IntByReference overflowRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        double value = CryptoLibrary.INSTANCE.cryptoAmountGetDouble(core, asUnit.core, overflowRef);
        return overflowRef.getValue() == BRCryptoBoolean.CRYPTO_TRUE ? Optional.absent() : Optional.of(value);
    }

    @Override
    public int compareTo(Amount o) {
        switch (CryptoLibrary.INSTANCE.cryptoAmountCompare(core, o.core)) {
            case BRCryptoComparison.CRYPTO_COMPARE_EQ: return 0;
            case BRCryptoComparison.CRYPTO_COMPARE_LT: return -1;
            case BRCryptoComparison.CRYPTO_COMPARE_GT: return 1;
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
        return BRCryptoComparison.CRYPTO_COMPARE_EQ == CryptoLibrary.INSTANCE.cryptoAmountCompare(core, amount.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core, unit);
    }
}
