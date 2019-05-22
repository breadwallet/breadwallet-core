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

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoCurrency;

import java.util.Objects;

public final class Currency {

    public static final String CODE_AS_BTC = "btc";
    public static final String CODE_AS_BCH = "bch";
    public static final String CODE_AS_ETH = "eth";

    /* package */
    final BRCryptoCurrency core;

    private final String uids;

    /* package */ Currency(String uids, String name, String code, String type) {
        this(CryptoLibrary.INSTANCE.cryptoCurrencyCreate(name, code, type), uids);
    }

    private Currency(BRCryptoCurrency core, String uids) {
        this.core = core;
        this.uids = uids;
    }

    @Override
    protected void finalize() {
        CryptoLibrary.INSTANCE.cryptoCurrencyGive(core);
    }

    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetName(core);
    }

    public String getCode() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetCode(core);
    }

    public String getType() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetType(core);
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
}
