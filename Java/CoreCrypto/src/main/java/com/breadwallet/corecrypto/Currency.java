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
import com.google.common.base.Optional;

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

    private final String uids;
    private final String name;
    private final String code;
    private final String type;

    @Nullable
    private final String issuer;

    private Currency(CoreBRCryptoCurrency core) {
        this.core = core;

        this.uids = core.getUids();
        this.name = core.getName();
        this.code = core.getCode();
        this.type = core.getType();
        this.issuer = core.getIssuer();
    }

    @Override
    public String getUids() {
        return uids;
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public String getCode() {
        return code;
    }

    @Override
    public String getType() {
        return type;
    }

    @Override
    public Optional<String> getIssuer() {
        return Optional.fromNullable(issuer);
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
