/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoSyncDepth {

    CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND {
        @Override
        public int toCore() {
            return CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND_VALUE;
        }
    },

    CRYPTO_SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK {
        @Override
        public int toCore() {
            return CRYPTO_SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK_VALUE;
        }
    },

    CRYPTO_SYNC_DEPTH_FROM_CREATION {
        @Override
        public int toCore() {
            return CRYPTO_SYNC_DEPTH_FROM_CREATION_VALUE;
        }
    };

    private static final int CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND_VALUE = 0;
    private static final int CRYPTO_SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK_VALUE = 1;
    private static final int CRYPTO_SYNC_DEPTH_FROM_CREATION_VALUE = 2;

    public static BRCryptoSyncDepth fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND_VALUE: return CRYPTO_SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND;
            case CRYPTO_SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK_VALUE:  return CRYPTO_SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK;
            case CRYPTO_SYNC_DEPTH_FROM_CREATION_VALUE:            return CRYPTO_SYNC_DEPTH_FROM_CREATION;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
