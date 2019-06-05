/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.CoreBRCryptoUnit;
import com.google.common.primitives.UnsignedInteger;

import java.util.Objects;

/* package */
final class Unit implements com.breadwallet.crypto.Unit {

    /* package */
    static Unit from(com.breadwallet.crypto.Unit unit) {
        if (unit instanceof Unit) {
            return (Unit) unit;
        }
        throw new IllegalArgumentException("Unsupported unit instance");
    }

    private final CoreBRCryptoUnit core;
    private final Unit base;
    private final String uids;
    private final Currency currency;

    /* package */
    Unit(Currency currency, String uids, String name, String symbol) {
        this(CoreBRCryptoUnit.createAsBase(currency.getCoreBRCryptoCurrency(), name, symbol), currency, uids, null);
    }

    /* package */
    Unit(Currency currency, String uids, String name, String symbol, Unit base, UnsignedInteger decimals) {
        this(CoreBRCryptoUnit.create(currency.getCoreBRCryptoCurrency(), name, symbol, base.core, decimals), currency, uids, base);
    }

    private Unit(CoreBRCryptoUnit core, Currency currency, String uids, Unit base) {
        this.core = core;
        this.currency = currency;
        this.uids = uids;
        this.base = base == null ? this : base;
    }

    @Override
    public Currency getCurrency() {
        return currency;
    }

    @Override
    public String getName() {
        return core.getName();
    }

    @Override
    public String getSymbol() {
        return core.getSymbol();
    }

    @Override
    public Unit getBase() {
        return base;
    }

    @Override
    public UnsignedInteger getDecimals() {
        return core.getDecimals();
    }

    @Override
    public boolean isCompatible(com.breadwallet.crypto.Unit other) {
        return core.isCompatible(from(other).core);
    }

    @Override
    public boolean hasCurrency(com.breadwallet.crypto.Currency currency) {
        return core.hasCurrency(Currency.from(currency).getCoreBRCryptoCurrency());
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
