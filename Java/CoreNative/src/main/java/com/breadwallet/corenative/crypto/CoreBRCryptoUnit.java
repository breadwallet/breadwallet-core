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

import java.util.BitSet;

public interface CoreBRCryptoUnit {

    static CoreBRCryptoUnit createAsBase(BRCryptoCurrency currency, String uids, String name, String symbol) {
        return new OwnedBRCryptoUnit(CryptoLibrary.INSTANCE.cryptoUnitCreateAsBase(currency, uids, name, symbol));
    }

    static CoreBRCryptoUnit create(BRCryptoCurrency currency, String uids, String name, String symbol, CoreBRCryptoUnit base, UnsignedInteger decimals) {
        BRCryptoUnit coreBaseUnit = base.asBRCryptoUnit();
        byte decimalsAsByte = UnsignedBytes.checkedCast(decimals.longValue());
        return new OwnedBRCryptoUnit(CryptoLibrary.INSTANCE.cryptoUnitCreate(currency, uids, name, symbol, coreBaseUnit, decimalsAsByte));
    }

    String getUids();

    String getName();

    String getSymbol();

    UnsignedInteger getDecimals();

    CoreBRCryptoUnit getBaseUnit();

    boolean isCompatible(CoreBRCryptoUnit other);

    BRCryptoCurrency getCurrency();

    boolean hasCurrency(BRCryptoCurrency currency);

    boolean isIdentical(CoreBRCryptoUnit other);

    BRCryptoUnit asBRCryptoUnit();
}
