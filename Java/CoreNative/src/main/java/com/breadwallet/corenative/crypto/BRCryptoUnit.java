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
import com.google.common.primitives.UnsignedBytes;
import com.google.common.primitives.UnsignedInteger;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoUnit extends PointerType {

    public static BRCryptoUnit createAsBase(BRCryptoCurrency currency, String uids, String name, String symbol) {
        return CryptoLibrary.INSTANCE.cryptoUnitCreateAsBase(currency, uids, name, symbol);
    }

    public static BRCryptoUnit create(BRCryptoCurrency currency, String uids, String name, String symbol, BRCryptoUnit base, UnsignedInteger decimals) {
        byte decimalsAsByte = UnsignedBytes.checkedCast(decimals.longValue());
        return CryptoLibrary.INSTANCE.cryptoUnitCreate(currency, uids, name, symbol, base, decimalsAsByte);
    }

    public BRCryptoUnit(Pointer address) {
        super(address);
    }

    public BRCryptoUnit() {
        super();
    }

    public String getUids() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetUids(this).getString(0, "UTF-8");
    }

    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetName(this).getString(0, "UTF-8");
    }

    public String getSymbol() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetSymbol(this).getString(0, "UTF-8");
    }

    public UnsignedInteger getDecimals() {
        return UnsignedInteger.fromIntBits(UnsignedBytes.toInt(CryptoLibrary.INSTANCE.cryptoUnitGetBaseDecimalOffset(this)));
    }

    public BRCryptoUnit getBaseUnit() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetBaseUnit(this);
    }

    public boolean isCompatible(BRCryptoUnit other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoUnitIsCompatible(this, other);
    }

    public BRCryptoCurrency getCurrency() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetCurrency(this);
    }

    public boolean hasCurrency(BRCryptoCurrency currency) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoUnitHasCurrency(this,  currency);
    }

    public boolean isIdentical(BRCryptoUnit other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoUnitIsIdentical(this, other);
    }

    public void give() {
        if (null != getPointer()) {
            CryptoLibrary.INSTANCE.cryptoUnitGive(this);
        }
    }
}
