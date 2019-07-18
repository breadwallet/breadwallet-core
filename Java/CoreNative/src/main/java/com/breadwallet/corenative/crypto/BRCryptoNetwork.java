/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoNetwork extends PointerType implements CoreBRCryptoNetwork {

    public BRCryptoNetwork(Pointer address) {
        super(address);
    }

    public BRCryptoNetwork() {
        super();
    }

    @Override
    public void setHeight(UnsignedLong height) {
        CryptoLibrary.INSTANCE.cryptoNetworkSetHeight(this, height.longValue());
    }

    @Override
    public CoreBRCryptoCurrency getCurrency() {
        return new OwnedBRCryptoCurrency(CryptoLibrary.INSTANCE.cryptoNetworkGetCurrency(this));
    }

    @Override
    public void setCurrency(CoreBRCryptoCurrency currency) {
        CryptoLibrary.INSTANCE.cryptoNetworkSetCurrency(this, currency.asBRCryptoCurrency());
    }

    @Override
    public boolean hasCurrency(CoreBRCryptoCurrency currency) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoNetworkHasCurrency(this, currency.asBRCryptoCurrency());
    }

    @Override
    public UnsignedLong getCurrencyCount() {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.cryptoNetworkGetCurrencyCount(this).longValue());
    }

    @Override
    public CoreBRCryptoCurrency getCurrency(UnsignedLong index) {
        return new OwnedBRCryptoCurrency(CryptoLibrary.INSTANCE.cryptoNetworkGetCurrencyAt(this,
                new SizeT(index.longValue())));
    }

    @Override
    public String getUids() {
        return CryptoLibrary.INSTANCE.cryptoNetworkGetUids(this).getString(0, "UTF-8");
    }

    @Override
    public boolean isMainnet() {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoNetworkIsMainnet(this);
    }

    @Override
    public UnsignedLong getHeight() {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.cryptoNetworkGetHeight(this));
    }

    @Override
    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoNetworkGetName(this).getString(0, "UTF-8");
    }

    @Override
    public int getType() {
        return CryptoLibrary.INSTANCE.cryptoNetworkGetType(this);
    }

    @Override
    public void addCurrency(CoreBRCryptoCurrency currency, CoreBRCryptoUnit baseUnit, CoreBRCryptoUnit defaultUnit) {
        CryptoLibrary.INSTANCE.cryptoNetworkAddCurrency(this, currency.asBRCryptoCurrency(), baseUnit.asBRCryptoUnit(), defaultUnit.asBRCryptoUnit());
    }

    @Override
    public void addCurrencyUnit(CoreBRCryptoCurrency currency, CoreBRCryptoUnit unit) {
        CryptoLibrary.INSTANCE.cryptoNetworkAddCurrencyUnit(this, currency.asBRCryptoCurrency(), unit.asBRCryptoUnit());
    }

    @Override
    public Optional<CoreBRCryptoUnit> getUnitAsBase(CoreBRCryptoCurrency currency) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoNetworkGetUnitAsBase(this, currency.asBRCryptoCurrency()));
    }

    @Override
    public Optional<CoreBRCryptoUnit> getUnitAsDefault(CoreBRCryptoCurrency currency) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoNetworkGetUnitAsDefault(this, currency.asBRCryptoCurrency()));
    }

    @Override
    public UnsignedLong getUnitCount(CoreBRCryptoCurrency currency) {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.cryptoNetworkGetUnitCount(this, currency.asBRCryptoCurrency()).longValue());
    }

    @Override
    public Optional<CoreBRCryptoUnit> getUnitAt(CoreBRCryptoCurrency currency, UnsignedLong index) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoNetworkGetUnitAt(this, currency.asBRCryptoCurrency(), new SizeT(index.longValue())));
    }

    @Override
    public BRCryptoNetwork asBRCryptoNetwork() {
        return this;
    }
}
