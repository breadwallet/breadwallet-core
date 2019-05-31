/*
 * Currency
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.libcrypto.crypto.CoreBRCryptoCurrency;

import java.util.Objects;

public final class CurrencyImpl implements Currency {

    /* package */
    static CurrencyImpl from(Currency currency) {
        if (currency instanceof CurrencyImpl) {
            return (CurrencyImpl) currency;
        }
        // TODO(fix): We should be able to create a CurrencyImpl from any Currency implementation
        throw new IllegalArgumentException("Unsupported currency instance");
    }

    private final CoreBRCryptoCurrency core;
    private final String uids;

    /* package */ CurrencyImpl(String uids, String name, String code, String type) {
        this(CoreBRCryptoCurrency.create(name, code, type), uids);
    }

    private CurrencyImpl(CoreBRCryptoCurrency core, String uids) {
        this.core = core;
        this.uids = uids;
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

        CurrencyImpl currency = (CurrencyImpl) o;
        return uids.equals(currency.uids);
    }

    @Override
    public int hashCode() {
        return Objects.hash(uids);
    }

    /* package */
    CoreBRCryptoCurrency getCoreBRCryptoCurrency() {
        return core;
    }
}
