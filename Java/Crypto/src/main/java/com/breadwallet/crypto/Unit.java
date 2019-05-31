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

import com.breadwallet.crypto.jni.crypto.CoreBRCryptoUnit;

import java.util.Objects;

public final class Unit {

    private final CoreBRCryptoUnit core;
    private final Unit base;
    private final String uids;
    private final Currency currency;

    /* package */
    Unit(Currency currency, String uids, String name, String symbol) {
        this(CoreBRCryptoUnit.createAsBase(currency.getCoreBRCryptoCurrency(), name, symbol), currency, uids, null);
    }

    /* package */
    Unit(Currency currency, String uids, String name, String symbol, Unit base, int decimals) {
        this(CoreBRCryptoUnit.create(currency.getCoreBRCryptoCurrency(), name, symbol, base.core, decimals), currency, uids, base);
    }

    private Unit(CoreBRCryptoUnit core, Currency currency, String uids, Unit base) {
        this.core = core;
        this.currency = currency;
        this.uids = uids;
        this.base = base == null ? this : base;
    }

    public Currency getCurrency() {
        return currency;
    }

    public String getName() {
        return core.getName();
    }

    public String getSymbol() {
        return core.getSymbol();
    }

    public Unit getBase() {
        return base;
    }

    public int getDecimals() {
        return core.getDecimals();
    }

    public boolean isCompatible(Unit other) {
        return core.isCompatible(other.core);
    }

    public boolean hasCurrency(Currency currency) {
        return core.hasCurrency(currency.getCoreBRCryptoCurrency());
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

    /* package */
    CoreBRCryptoUnit getCoreBRCryptoUnit() {
        return core;
    }
}
