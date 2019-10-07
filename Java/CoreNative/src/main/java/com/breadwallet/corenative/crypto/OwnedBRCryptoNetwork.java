/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.List;
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
    public BRCryptoCurrency getCurrency() {
        return core.getCurrency();
    }

    @Override
    public void setCurrency(BRCryptoCurrency other) {
        core.setCurrency(other);
    }

    @Override
    public boolean hasCurrency(BRCryptoCurrency currency) {
        return core.hasCurrency(currency);
    }

    @Override
    public UnsignedLong getCurrencyCount() {
        return core.getCurrencyCount();
    }

    @Override
    public BRCryptoCurrency getCurrency(UnsignedLong index) {
        return core.getCurrency(index);
    }

    @Override
    public List<CoreBRCryptoNetworkFee> getFees() {
        return core.getFees();
    }

    @Override
    public void setFees(List<CoreBRCryptoNetworkFee> fees) {
        core.setFees(fees);
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
    public void setHeight(UnsignedLong height) {
        core.setHeight(height);
    }

    @Override
    public UnsignedInteger getConfirmationsUntilFinal() {
        return core.getConfirmationsUntilFinal();
    }

    @Override
    public void setConfirmationsUntilFinal(UnsignedInteger confirmationsUntilFinal) {
        core.setConfirmationsUntilFinal(confirmationsUntilFinal);
    }

    @Override
    public String getName() {
        return core.getName();
    }

    @Override
    public void addFee(CoreBRCryptoNetworkFee fee) {
        core.addFee(fee);
    }

    @Override
    public void addCurrency(BRCryptoCurrency currency, CoreBRCryptoUnit baseUnit, CoreBRCryptoUnit defaultUnit) {
        core.addCurrency(currency, baseUnit, defaultUnit);
    }

    @Override
    public void addCurrencyUnit(BRCryptoCurrency currency, CoreBRCryptoUnit unit) {
        core.addCurrencyUnit(currency, unit);
    }

    @Override
    public Optional<CoreBRCryptoUnit> getUnitAsBase(BRCryptoCurrency currency) {
        return core.getUnitAsBase(currency);
    }

    @Override
    public Optional<CoreBRCryptoUnit> getUnitAsDefault(BRCryptoCurrency currency) {
        return core.getUnitAsDefault(currency);
    }

    @Override
    public UnsignedLong getUnitCount(BRCryptoCurrency currency) {
        return core.getUnitCount(currency);
    }

    @Override
    public Optional<CoreBRCryptoUnit> getUnitAt(BRCryptoCurrency currency, UnsignedLong index) {
        return core.getUnitAt(currency, index);
    }

    @Override
    public Optional<BRCryptoAddress> addressFor(String address) {
        return core.addressFor(address);
    }

    @Override
    public BRCryptoNetwork asBRCryptoNetwork() {
        return core;
    }

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
