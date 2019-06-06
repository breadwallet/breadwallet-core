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

public interface Currency {

    String CODE_AS_BTC = "btc";
    String CODE_AS_BCH = "bch";
    String CODE_AS_ETH = "eth";

    String getName();

    String getCode();

    String getType();

    boolean equals(Object o);

    int hashCode();
}
