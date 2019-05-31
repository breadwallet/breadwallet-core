/*
 * Currency
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.crypto.CoreBRCryptoCurrency;

import java.util.Objects;

public final class Currency {

    public static final String CODE_AS_BTC = "btc";
    public static final String CODE_AS_BCH = "bch";
    public static final String CODE_AS_ETH = "eth";

    private final CoreBRCryptoCurrency core;
    private final String uids;

    /* package */ Currency(String uids, String name, String code, String type) {
        this(CoreBRCryptoCurrency.create(name, code, type), uids);
    }

    private Currency(CoreBRCryptoCurrency core, String uids) {
        this.core = core;
        this.uids = uids;
    }

    public String getName() {
        return core.getName();
    }

    public String getCode() {
        return core.getCode();
    }

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
