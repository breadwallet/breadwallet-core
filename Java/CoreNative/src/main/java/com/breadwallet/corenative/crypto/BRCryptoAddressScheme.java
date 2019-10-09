/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.google.common.collect.ImmutableMap;

import static com.google.common.base.Preconditions.checkState;

public enum BRCryptoAddressScheme {

    CRYPTO_ADDRESS_SCHEME_BTC_LEGACY {
        @Override
        public int toNative() {
            return 0;
        }
    },

    CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT {
        @Override
        public int toNative() {
            return 1;
        }
    },

    CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT {
        @Override
        public int toNative() {
            return 2;
        }
    },

    CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT {
        @Override
        public int toNative() {
            return 3;
        }
    };

    private static final ImmutableMap<Integer, BRCryptoAddressScheme> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRCryptoAddressScheme> b = ImmutableMap.builder();

        b.put(CRYPTO_ADDRESS_SCHEME_BTC_LEGACY.toNative(),  CRYPTO_ADDRESS_SCHEME_BTC_LEGACY);
        b.put(CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT.toNative(),  CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT);
        b.put(CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT.toNative(), CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT);
        b.put(CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT.toNative(), CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT);

        LOOKUP = b.build();
    }

    public static BRCryptoAddressScheme fromNative(int nativeValue) {
        BRCryptoAddressScheme status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
