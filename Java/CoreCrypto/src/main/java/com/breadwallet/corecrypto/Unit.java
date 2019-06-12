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
        if (unit instanceof Unit) {
            return (Unit) unit;
        }
        throw new IllegalArgumentException("Unsupported unit instance");
    }

    private final CoreBRCryptoUnit core;

    private Unit(CoreBRCryptoUnit core) {
        this.core = core;
    }

    @Override
    public Currency getCurrency() {
        return Currency.create(core.getCurrency());
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
        return core.getBase().transform(u -> new Unit(u)).or(this);
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
        return core.isIdentical(unit.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core.getUids());
    }

    /* package */
    CoreBRCryptoUnit getCoreBRCryptoUnit() {
        return core;
    }
}
