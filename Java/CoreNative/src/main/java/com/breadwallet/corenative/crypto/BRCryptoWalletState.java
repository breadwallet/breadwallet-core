/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoWalletState {

    CRYPTO_WALLET_STATE_CREATED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_STATE_CREATED_VALUE;
        }
    },

    CRYPTO_WALLET_STATE_DELETED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_STATE_DELETED_VALUE;
        }
    };

    private static final int CRYPTO_WALLET_STATE_CREATED_VALUE = 0;
    private static final int CRYPTO_WALLET_STATE_DELETED_VALUE = 1;

    public static BRCryptoWalletState fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_WALLET_STATE_CREATED_VALUE: return CRYPTO_WALLET_STATE_CREATED;
            case CRYPTO_WALLET_STATE_DELETED_VALUE: return CRYPTO_WALLET_STATE_DELETED;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
