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
import com.google.common.primitives.UnsignedInteger;

import java.util.Objects;

/* package */
class OwnedBRCryptoUnit implements CoreBRCryptoUnit {

    private final BRCryptoUnit core;

    /* package */
    OwnedBRCryptoUnit(BRCryptoUnit core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoUnitGive(core);
        }
    }

    @Override
    public String getUids() {
        return core.getUids();
    }

    @Override
    public String getName() {
        return core.getName();
    }

    @Override
    public String getSymbol() {
        return core.getSymbol();
    }

    @Override
    public UnsignedInteger getDecimals() {
        return core.getDecimals();
    }

    @Override
    public CoreBRCryptoUnit getBase() {
        return core.getBase();
    }

    @Override
    public boolean isCompatible(CoreBRCryptoUnit other) {
        return core.isCompatible(other);
    }

    @Override
    public CoreBRCryptoCurrency getCurrency() {
        return core.getCurrency();
    }

    @Override
    public boolean hasCurrency(CoreBRCryptoCurrency currency) {
        return core.hasCurrency(currency);
    }

    @Override
    public boolean isIdentical(CoreBRCryptoUnit other) {
        return core.isIdentical(other);
    }

    @Override
    public BRCryptoUnit asBRCryptoUnit() {
        return core;
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoUnit) {
            OwnedBRCryptoUnit that = (OwnedBRCryptoUnit) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoUnit) {
            BRCryptoUnit that = (BRCryptoUnit) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
