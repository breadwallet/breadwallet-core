/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoSyncMode {

    CRYPTO_SYNC_MODE_API_ONLY {
        @Override
        public int toCore() {
            return CRYPTO_SYNC_MODE_API_ONLY_VALUE;
        }
    },

    CRYPTO_SYNC_MODE_API_WITH_P2P_SEND {
        @Override
        public int toCore() {
            return CRYPTO_SYNC_MODE_API_WITH_P2P_SEND_VALUE;
        }
    },

    CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC {
        @Override
        public int toCore() {
            return CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC_VALUE;
        }
    },

    CRYPTO_SYNC_MODE_P2P_ONLY {
        @Override
        public int toCore() {
            return CRYPTO_SYNC_MODE_P2P_ONLY_VALUE;
        }
    };

    private static final int CRYPTO_SYNC_MODE_API_ONLY_VALUE = 0;
    private static final int CRYPTO_SYNC_MODE_API_WITH_P2P_SEND_VALUE = 1;
    private static final int CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC_VALUE = 2;
    private static final int CRYPTO_SYNC_MODE_P2P_ONLY_VALUE = 3;

    public static BRCryptoSyncMode fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_SYNC_MODE_API_ONLY_VALUE:          return CRYPTO_SYNC_MODE_API_ONLY;
            case CRYPTO_SYNC_MODE_API_WITH_P2P_SEND_VALUE: return CRYPTO_SYNC_MODE_API_WITH_P2P_SEND;
            case CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC_VALUE: return CRYPTO_SYNC_MODE_P2P_WITH_API_SYNC;
            case CRYPTO_SYNC_MODE_P2P_ONLY_VALUE:          return CRYPTO_SYNC_MODE_P2P_ONLY;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
