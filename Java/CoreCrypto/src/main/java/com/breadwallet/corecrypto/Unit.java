/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
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
    static Unit create(CoreBRCryptoUnit core) {
        return new Unit(core);
    }

    /* package */
    static Unit create(Currency currency, String uids, String name, String symbol) {
        return new Unit(CoreBRCryptoUnit.createAsBase(currency.getCoreBRCryptoCurrency(), uids, name, symbol));
    }

    /* package */
    static Unit create(Currency currency, String uids, String name, String symbol, Unit base, UnsignedInteger decimals) {
        return new Unit(CoreBRCryptoUnit.create(currency.getCoreBRCryptoCurrency(), uids, name, symbol, base.core, decimals));
    }

    /* package */
    static Unit from(com.breadwallet.crypto.Unit unit) {
        if (unit == null) {
            return null;
        }

        if (unit instanceof Unit) {
            return (Unit) unit;
        }

        throw new IllegalArgumentException("Unsupported unit instance");
    }

    private final CoreBRCryptoUnit core;

    private final Currency currency;
    private final String name;
    private final String symbol;
    private final String uids;
    private final UnsignedInteger decimals;

    private Unit(CoreBRCryptoUnit core) {
        this.core = core;

        // don't cache base unit to avoid recursion; cost of get is cheap
        this.currency = Currency.create(core.getCurrency());
        this.name = core.getName();
        this.symbol = core.getSymbol();
        this.uids = core.getUids();
        this.decimals = core.getDecimals();
    }

    @Override
    public Currency getCurrency() {
        return currency;
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public String getSymbol() {
        return symbol;
    }

    @Override
    public Unit getBase() {
        return new Unit(core.getBaseUnit());
    }

    @Override
    public UnsignedInteger getDecimals() {
        return decimals;
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
        return core.isIdentical(unit.core);
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
