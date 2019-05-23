/*
 * Unit
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoBoolean;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoUnit;
import com.google.common.primitives.UnsignedBytes;

import java.util.Objects;

public final class Unit {

    // TODO: This is package, which is risky since it can be free'd by the finalize code here
    /* package */
    final BRCryptoUnit core;

    private Unit base;
    private final String uids;
    private final Currency currency;

    /* package */ Unit(Currency currency, String uids, String name, String symbol) {
        this(CryptoLibrary.INSTANCE.cryptoUnitCreateAsBase(currency.core, name, symbol), currency, uids, null);
    }

    /* package */ Unit(Currency currency, String uids, String name, String symbol, Unit base, int decimals) {
        this(CryptoLibrary.INSTANCE.cryptoUnitCreate(currency.core, name, symbol, base.core,
                UnsignedBytes.checkedCast(decimals)), currency, uids, base);
    }

    private Unit(BRCryptoUnit core, Currency currency, String uids, Unit base) {
        this.core = core;
        this.currency = currency;
        this.uids = uids;
        this.base = base == null ? this : base;
    }

    @Override
    protected void finalize() {
        CryptoLibrary.INSTANCE.cryptoUnitGive(core);
    }

    public Currency getCurrency() {
        return currency;
    }

    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetName(core);
    }

    public String getSymbol() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetSymbol(core);
    }

    public Unit getBase() {
        return base;
    }

    public int getDecimals() {
        return UnsignedBytes.toInt(CryptoLibrary.INSTANCE.cryptoUnitGetBaseDecimalOffset(core));
    }

    public boolean isCompatible(Unit other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoUnitIsCompatible(this.core, other.core);
    }

    public boolean hasCurrency(Currency currency) {
        return currency.core.equals(CryptoLibrary.INSTANCE.cryptoUnitGetCurrency(core));
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }

        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        Unit unit = (Unit) o;
        return uids.equals(unit.uids);
    }

    @Override
    public int hashCode() {
        return Objects.hash(uids);
    }
}
