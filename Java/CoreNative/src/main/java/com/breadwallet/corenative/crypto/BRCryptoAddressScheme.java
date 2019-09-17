/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public interface BRCryptoAddressScheme {

    int CRYPTO_ADDRESS_SCHEME_BTC_LEGACY = 0;
    int CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT = 1;
    int CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT = 2;
    int CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT = 3;
}
