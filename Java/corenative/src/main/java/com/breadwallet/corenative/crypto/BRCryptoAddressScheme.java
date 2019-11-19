/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoAddressScheme {

    CRYPTO_ADDRESS_SCHEME_BTC_LEGACY {
        @Override
        public int toCore() {
            return CRYPTO_ADDRESS_SCHEME_BTC_LEGACY_VALUE;
        }
    },

    CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT {
        @Override
        public int toCore() {
            return CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT_VALUE;
        }
    },

    CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT {
        @Override
        public int toCore() {
            return CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT_VALUE;
        }
    },

    CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT {
        @Override
        public int toCore() {
            return CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT_VALUE;
        }
    };

    private static final int CRYPTO_ADDRESS_SCHEME_BTC_LEGACY_VALUE  = 0;
    private static final int CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT_VALUE  = 1;
    private static final int CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT_VALUE = 2;
    private static final int CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT_VALUE = 3;

    public static BRCryptoAddressScheme fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_ADDRESS_SCHEME_BTC_LEGACY_VALUE:  return CRYPTO_ADDRESS_SCHEME_BTC_LEGACY;
            case CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT_VALUE:  return CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT;
            case CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT_VALUE: return CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT;
            case CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT_VALUE: return CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
