/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

/**
 * An AddressScheme determines the from of wallet-generated address.  For example, a Bitcoin wallet
 * can have a 'Segwit/BECH32' address scheme or a 'Legacy' address scheme.  The address, which is
 * ultimately a sequence of bytes, gets formatted as a string based on the scheme.
 *
 * The WalletManager holds an array of AddressSchemes as well as the preferred AddressScheme.
 */
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
