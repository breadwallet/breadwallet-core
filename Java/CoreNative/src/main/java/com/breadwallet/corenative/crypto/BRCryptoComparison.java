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

public enum BRCryptoComparison {

    CRYPTO_COMPARE_LT {
        @Override
        public int toNative() {
            return 0;
        }
    },

    CRYPTO_COMPARE_EQ {
        @Override
        public int toNative() {
            return 1;
        }
    },

    CRYPTO_COMPARE_GT {
        @Override
        public int toNative() {
            return 2;
        }
    };

    private static final ImmutableMap<Integer, BRCryptoComparison> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRCryptoComparison> b = ImmutableMap.builder();

        b.put(CRYPTO_COMPARE_LT.toNative(), CRYPTO_COMPARE_LT);
        b.put(CRYPTO_COMPARE_EQ.toNative(), CRYPTO_COMPARE_EQ);
        b.put(CRYPTO_COMPARE_GT.toNative(), CRYPTO_COMPARE_GT);

        LOOKUP = b.build();
    }

    public static BRCryptoComparison fromNative(int nativeValue) {
        BRCryptoComparison status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
