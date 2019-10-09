/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoComparison {

    CRYPTO_COMPARE_LT {
        @Override
        public int toCore() {
            return CRYPTO_COMPARE_LT_VALUE;
        }
    },

    CRYPTO_COMPARE_EQ {
        @Override
        public int toCore() {
            return CRYPTO_COMPARE_EQ_VALUE;
        }
    },

    CRYPTO_COMPARE_GT {
        @Override
        public int toCore() {
            return CRYPTO_COMPARE_GT_VALUE;
        }
    };

    private static final int CRYPTO_COMPARE_LT_VALUE = 0;
    private static final int CRYPTO_COMPARE_EQ_VALUE = 1;
    private static final int CRYPTO_COMPARE_GT_VALUE = 2;

    public static BRCryptoComparison fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_COMPARE_LT_VALUE: return CRYPTO_COMPARE_LT;
            case CRYPTO_COMPARE_EQ_VALUE: return CRYPTO_COMPARE_EQ;
            case CRYPTO_COMPARE_GT_VALUE: return CRYPTO_COMPARE_GT;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
