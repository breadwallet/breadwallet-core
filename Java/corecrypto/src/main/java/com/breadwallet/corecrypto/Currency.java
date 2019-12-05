/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoCurrency;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.util.Objects;

/* package */
final class Currency implements com.breadwallet.crypto.Currency {

    /* package */
    static Currency create (String uids, String name, String code, String type, @Nullable String issuer) {
        BRCryptoCurrency core = BRCryptoCurrency.create(uids, name, code, type, issuer);
        return Currency.create(core);
    }

    /* package */
    static Currency create(BRCryptoCurrency core) {
        Currency currency = new Currency(core);
        ReferenceCleaner.register(currency, core::give);
        return currency;
    }

    /* package */
    static Currency from(com.breadwallet.crypto.Currency currency) {
        if (currency == null) {
            return null;
        }

        if (currency instanceof Currency) {
            return (Currency) currency;
        }

        throw new IllegalArgumentException("Unsupported currency instance");
    }

    private final BRCryptoCurrency core;

    private final Supplier<String> uidsSupplier;
    private final Supplier<String> nameSupplier;
    private final Supplier<String> codeSupplier;
    private final Supplier<String> typeSupplier;

    private final Supplier<String> issuerSupplier;

    private Currency(BRCryptoCurrency core) {
        this.core = core;

        this.uidsSupplier = Suppliers.memoize(core::getUids);
        this.nameSupplier = Suppliers.memoize(core::getName);
        this.codeSupplier = Suppliers.memoize(core::getCode);
        this.typeSupplier = Suppliers.memoize(core::getType);
        this.issuerSupplier = Suppliers.memoize(core::getIssuer);
    }

    @Override
    public String getUids() {
        return uidsSupplier.get();
    }

    @Override
    public String getName() {
        return nameSupplier.get();
    }

    @Override
    public String getCode() {
        return codeSupplier.get();
    }

    @Override
    public String getType() {
        return typeSupplier.get();
    }

    @Override
    public Optional<String> getIssuer() {
        return Optional.fromNullable(issuerSupplier.get());
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
    BRCryptoCurrency getCoreBRCryptoCurrency() {
        return core;
    }
}
