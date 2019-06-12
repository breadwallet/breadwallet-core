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

import java.util.BitSet;

public interface CoreBRCryptoUnit {

    static CoreBRCryptoUnit createAsBase(CoreBRCryptoCurrency currency, String uids, String name, String symbol) {
        BRCryptoCurrency coreCurrency = currency.asBRCryptoCurrency();
        return new OwnedBRCryptoUnit(CryptoLibrary.INSTANCE.cryptoUnitCreateAsBase(coreCurrency, uids, name, symbol));
    }

    static CoreBRCryptoUnit create(CoreBRCryptoCurrency currency, String uids, String name, String symbol, CoreBRCryptoUnit base, UnsignedInteger decimals) {
        BRCryptoCurrency coreCurrency = currency.asBRCryptoCurrency();
        BRCryptoUnit coreBaseUnit = base.asBRCryptoUnit();
        byte decimalsAsByte = UnsignedBytes.checkedCast(decimals.longValue());
        return new OwnedBRCryptoUnit(CryptoLibrary.INSTANCE.cryptoUnitCreate(coreCurrency, uids, name, symbol, coreBaseUnit, decimalsAsByte));
    }

    String getUids();

    String getName();

    String getSymbol();

    UnsignedInteger getDecimals();

    Optional<CoreBRCryptoUnit> getBase();

    boolean isCompatible(CoreBRCryptoUnit other);

    boolean hasCurrency(CoreBRCryptoCurrency currency);

    boolean isIdentical(CoreBRCryptoUnit other);

    BRCryptoUnit asBRCryptoUnit();
}
