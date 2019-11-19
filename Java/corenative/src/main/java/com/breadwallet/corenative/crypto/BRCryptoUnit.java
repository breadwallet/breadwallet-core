/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.google.common.primitives.UnsignedBytes;
import com.google.common.primitives.UnsignedInteger;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoUnit extends PointerType {

    public static BRCryptoUnit createAsBase(BRCryptoCurrency currency, String uids, String name, String symbol) {
        return new BRCryptoUnit(
                CryptoLibraryDirect.cryptoUnitCreateAsBase(
                        currency.getPointer(),
                        uids,
                        name,
                        symbol
                )
        );
    }

    public static BRCryptoUnit create(BRCryptoCurrency currency, String uids, String name, String symbol, BRCryptoUnit base, UnsignedInteger decimals) {
        byte decimalsAsByte = UnsignedBytes.checkedCast(decimals.longValue());
        return new BRCryptoUnit(
                CryptoLibraryDirect.cryptoUnitCreate(
                        currency.getPointer(),
                        uids,
                        name,
                        symbol,
                        base.getPointer(),
                        decimalsAsByte
                )
        );
    }

    public BRCryptoUnit() {
        super();
    }

    public BRCryptoUnit(Pointer address) {
        super(address);
    }

    public String getUids() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoUnitGetUids(thisPtr).getString(0, "UTF-8");
    }

    public String getName() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoUnitGetName(thisPtr).getString(0, "UTF-8");
    }

    public String getSymbol() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoUnitGetSymbol(thisPtr).getString(0, "UTF-8");
    }

    public UnsignedInteger getDecimals() {
        Pointer thisPtr = this.getPointer();

        return UnsignedInteger.fromIntBits(UnsignedBytes.toInt(CryptoLibraryDirect.cryptoUnitGetBaseDecimalOffset(thisPtr)));
    }

    public BRCryptoUnit getBaseUnit() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoUnit(CryptoLibraryDirect.cryptoUnitGetBaseUnit(thisPtr));
    }

    public BRCryptoCurrency getCurrency() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoCurrency(CryptoLibraryDirect.cryptoUnitGetCurrency(thisPtr));
    }

    public boolean hasCurrency(BRCryptoCurrency currency) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoUnitHasCurrency(thisPtr,  currency.getPointer());
    }

    public boolean isCompatible(BRCryptoUnit other) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoUnitIsCompatible(thisPtr, other.getPointer());
    }

    public boolean isIdentical(BRCryptoUnit other) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoUnitIsIdentical(thisPtr, other.getPointer());
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoUnitGive(thisPtr);
    }
}
