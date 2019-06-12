/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
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
    public Optional<Double> getDouble(CoreBRCryptoUnit unit) {
        return core.getDouble(unit);
    }

    @Override
    public Optional<UnsignedLong> getIntegerRaw() {
        return core.getIntegerRaw();
    }

    @Override
    public CoreBRCryptoAmount add(CoreBRCryptoAmount amount) {
        return new OwnedBRCryptoAmount(core.add(amount));
    }

    @Override
    public CoreBRCryptoAmount sub(CoreBRCryptoAmount amount) {
        return new OwnedBRCryptoAmount(core.sub(amount));
    }

    @Override
    public CoreBRCryptoAmount negate() {
        return new OwnedBRCryptoAmount(core.negate());
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
    public String toStringWithBase(int base, String preface) {
        return core.toStringWithBase(base, preface);
    }

    @Override
    public BRCryptoAmount asBRCryptoAmount() {
        return core;
    }

    // TODO(discuss): Do we want to do a value comparison?
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
