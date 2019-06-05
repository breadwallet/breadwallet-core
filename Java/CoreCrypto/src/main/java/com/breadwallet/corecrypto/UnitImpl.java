/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.Unit;
import com.breadwallet.corenative.crypto.CoreBRCryptoUnit;
import com.google.common.primitives.UnsignedInteger;

import java.util.Objects;

/* package */
final class UnitImpl implements Unit {

    /* package */
    static UnitImpl from(Unit unit) {
        if (unit instanceof UnitImpl) {
            return (UnitImpl) unit;
        }
        throw new IllegalArgumentException("Unsupported unit instance");
    }

    private final CoreBRCryptoUnit core;
    private final UnitImpl base;
    private final String uids;
    private final CurrencyImpl currency;

    /* package */
    UnitImpl(CurrencyImpl currency, String uids, String name, String symbol) {
        this(CoreBRCryptoUnit.createAsBase(currency.getCoreBRCryptoCurrency(), name, symbol), currency, uids, null);
    }

    /* package */
    UnitImpl(CurrencyImpl currency, String uids, String name, String symbol, UnitImpl base, UnsignedInteger decimals) {
        this(CoreBRCryptoUnit.create(currency.getCoreBRCryptoCurrency(), name, symbol, base.core, decimals), currency, uids, base);
    }

    private UnitImpl(CoreBRCryptoUnit core, CurrencyImpl currency, String uids, UnitImpl base) {
        this.core = core;
        this.currency = currency;
        this.uids = uids;
        this.base = base == null ? this : base;
    }

    @Override
    public CurrencyImpl getCurrency() {
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
    public UnitImpl getBase() {
        return base;
    }

    @Override
    public UnsignedInteger getDecimals() {
        return core.getDecimals();
    }

    @Override
    public boolean isCompatible(Unit other) {
        UnitImpl otherImpl = from(other);
        return core.isCompatible(otherImpl.core);
    }

    @Override
    public boolean hasCurrency(Currency currency) {
        CurrencyImpl currencyImpl = CurrencyImpl.from(currency);
        return core.hasCurrency(currencyImpl.getCoreBRCryptoCurrency());
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }

        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        UnitImpl unit = (UnitImpl) o;
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
