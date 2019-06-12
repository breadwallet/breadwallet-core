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
import com.google.common.primitives.UnsignedBytes;
import com.google.common.primitives.UnsignedInteger;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoUnit extends PointerType implements CoreBRCryptoUnit {

    public BRCryptoUnit(Pointer address) {
        super(address);
    }

    public BRCryptoUnit() {
        super();
    }

    @Override
    public BRCryptoUnit asBRCryptoUnit() {
        return this;
    }


    @Override
    public String getUids() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetUids(this).getString(0, "UTF-8");
    }

    @Override
    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetName(this).getString(0, "UTF-8");
    }

    @Override
    public String getSymbol() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetSymbol(this).getString(0, "UTF-8");
    }

    @Override
    public UnsignedInteger getDecimals() {
        return UnsignedInteger.fromIntBits(UnsignedBytes.toInt(CryptoLibrary.INSTANCE.cryptoUnitGetBaseDecimalOffset(this)));
    }

    @Override
    public Optional<CoreBRCryptoUnit> getBase() {
        BRCryptoUnit baseUnit = CryptoLibrary.INSTANCE.cryptoUnitGetBaseUnit(this);
        if (null != baseUnit) {
            baseUnit = CryptoLibrary.INSTANCE.cryptoUnitTake(baseUnit);
        }
        return Optional.fromNullable(baseUnit).transform(OwnedBRCryptoUnit::new);
    }

    @Override
    public boolean isCompatible(CoreBRCryptoUnit other) {
        BRCryptoUnit otherCore = other.asBRCryptoUnit();
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoUnitIsCompatible(this, otherCore);
    }

    @Override
    public boolean hasCurrency(CoreBRCryptoCurrency currency) {
        BRCryptoCurrency coreCurrency = currency.asBRCryptoCurrency();
        return coreCurrency.equals(CryptoLibrary.INSTANCE.cryptoUnitGetCurrency(this));
    }

    @Override
    public boolean isIdentical(CoreBRCryptoUnit other) {
        BRCryptoUnit otherCore = other.asBRCryptoUnit();
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoUnitIsIdentical(this, otherCore);
    }
}
