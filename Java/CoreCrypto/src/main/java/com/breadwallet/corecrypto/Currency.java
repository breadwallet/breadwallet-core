/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.crypto.CoreBRCryptoCurrency;

import java.util.Objects;

/* package */
final class Currency implements com.breadwallet.crypto.Currency {

    /* package */
    static Currency create(CoreBRCryptoCurrency core) {
        return new Currency(core);
    }

    /* package */
    static Currency create (String uids, String name, String code, String type, @Nullable String issuer) {
        return new Currency(CoreBRCryptoCurrency.create(uids, name, code, type, issuer));
    }

    /* package */
    static Currency from(com.breadwallet.crypto.Currency currency) {
        if (currency instanceof Currency) {
            return (Currency) currency;
        }
        throw new IllegalArgumentException("Unsupported currency instance");
    }

    private final CoreBRCryptoCurrency core;

    private Currency(CoreBRCryptoCurrency core) {
        this.core = core;
    }

    @Override
    public String getUids() {
        return core.getUids();
    }

    @Override
    public String getName() {
        return core.getName();
    }

    @Override
    public String getCode() {
        return core.getCode();
    }

    @Override
    public String getType() {
        return core.getType();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }

        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        Currency currency = (Currency) o;
        return core.isIdentical(currency.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(getUids());
    }

    /* package */
    CoreBRCryptoCurrency getCoreBRCryptoCurrency() {
        return core;
    }
}
