/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

public enum  AddressScheme {
    BTC_LEGACY,
    BTC_SEGWIT,
    ETH_DEFAULT,
    GEN_DEFAULT;

    @Override
    public String toString() {
        switch (this) {
            case BTC_LEGACY:
                return "BTC Legacy";
            case BTC_SEGWIT:
                return "BTC Segwit";
            case ETH_DEFAULT:
                return "ETH Default";
            case GEN_DEFAULT:
                return "GEN Default";
            default:
                return "Default";
        }
    }
}
