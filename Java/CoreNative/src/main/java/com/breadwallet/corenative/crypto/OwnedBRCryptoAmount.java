/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.ptr.IntByReference;

import java.util.Objects;

/* package */
class OwnedBRCryptoAmount implements CoreBRCryptoAmount {

    private final BRCryptoAmount core;

    /* package */
    OwnedBRCryptoAmount(BRCryptoAmount core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoAmountGive(core);
        }
    }

    @Override
    public CoreBRCryptoCurrency getCurrency() {
        return core.getCurrency();
    }

    @Override
    public CoreBRCryptoUnit getUnit() {
        return core.getUnit();
    }

    @Override
    public Optional<Double> getDouble(CoreBRCryptoUnit unit) {
        return core.getDouble(unit);
    }

    @Override
    public Optional<CoreBRCryptoAmount> add(CoreBRCryptoAmount amount) {
        return core.add(amount);
    }

    @Override
    public Optional<CoreBRCryptoAmount> sub(CoreBRCryptoAmount amount) {
        return core.sub(amount);
    }

    @Override
    public CoreBRCryptoAmount negate() {
        return core.negate();
    }

    @Override
    public Optional<CoreBRCryptoAmount> convert(CoreBRCryptoUnit toUnit) {
        return core.convert(toUnit);
    }

    @Override
    public boolean isNegative() {
        return core.isNegative();
    }

    @Override
    public int compare(CoreBRCryptoAmount o) {
        return core.compare(o);
    }

    @Override
    public boolean isCompatible(CoreBRCryptoAmount o) {
        return core.isCompatible(o);
    }

    @Override
    public boolean hasCurrency(CoreBRCryptoCurrency o) {
        return core.hasCurrency(o);
    }

    @Override
    public String toStringWithBase(int base, String preface) {
        return core.toStringWithBase(base, preface);
    }

    @Override
    public BRCryptoAmount asBRCryptoAmount() {
        return core;
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoAmount) {
            OwnedBRCryptoAmount that = (OwnedBRCryptoAmount) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoAmount) {
            BRCryptoAmount that = (BRCryptoAmount) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
