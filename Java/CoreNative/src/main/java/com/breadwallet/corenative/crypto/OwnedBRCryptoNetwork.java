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

import java.util.Objects;

/* package */
class OwnedBRCryptoNetwork implements CoreBRCryptoNetwork {

    private final BRCryptoNetwork core;

    /* package */
    OwnedBRCryptoNetwork(BRCryptoNetwork core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoNetworkGive(core);
        }
    }

    @Override
    public void setHeight(UnsignedLong height) {
        core.setHeight(height);
    }

    @Override
    public CoreBRCryptoCurrency getCurrency() {
        return core.getCurrency();
    }

    @Override
    public void setCurrency(CoreBRCryptoCurrency other) {
        core.setCurrency(other);
    }

    @Override
    public boolean hasCurrency(CoreBRCryptoCurrency currency) {
        return core.hasCurrency(currency);
    }

    @Override
    public String getUids() {
        return core.getUids();
    }

    @Override
    public boolean isMainnet() {
        return core.isMainnet();
    }

    @Override
    public UnsignedLong getHeight() {
        return core.getHeight();
    }

    @Override
    public String getName() {
        return core.getName();
    }

    @Override
    public int getType() {
        return core.getType();
    }

    @Override
    public void addCurrency(CoreBRCryptoCurrency currency, CoreBRCryptoUnit baseUnit, CoreBRCryptoUnit defaultUnit) {
        core.addCurrency(currency, baseUnit, defaultUnit);
    }

    @Override
    public void addCurrencyUnit(CoreBRCryptoCurrency currency, CoreBRCryptoUnit unit) {
        core.addCurrencyUnit(currency, unit);
    }

    @Override
    public Optional<CoreBRCryptoUnit> getUnitAsBase(CoreBRCryptoCurrency currency) {
        return core.getUnitAsBase(currency);
    }

    @Override
    public Optional<CoreBRCryptoUnit> getUnitAsDefault(CoreBRCryptoCurrency currency) {
        return core.getUnitAsDefault(currency);
    }

    @Override
    public UnsignedLong getUnitCount(CoreBRCryptoCurrency currency) {
        return core.getUnitCount(currency);
    }

    @Override
    public Optional<CoreBRCryptoUnit> getUnitAt(CoreBRCryptoCurrency currency, UnsignedLong index) {
        return core.getUnitAt(currency, index);
    }

    @Override
    public BRCryptoNetwork asBRCryptoNetwork() {
        return core;
    }

    // TODO(discuss): Do we want to do a value comparison?
    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoNetwork) {
            OwnedBRCryptoNetwork that = (OwnedBRCryptoNetwork) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoNetwork) {
            BRCryptoNetwork that = (BRCryptoNetwork) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
