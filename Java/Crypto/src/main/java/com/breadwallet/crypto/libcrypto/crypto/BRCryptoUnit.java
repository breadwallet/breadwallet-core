/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.crypto;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
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
    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetName(this);
    }

    @Override
    public String getSymbol() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetSymbol(this);
    }

    @Override
    public UnsignedInteger getDecimals() {
        return UnsignedInteger.fromIntBits(UnsignedBytes.toInt(CryptoLibrary.INSTANCE.cryptoUnitGetBaseDecimalOffset(this)));
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
}
