/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoWalletManagerStateType {

    CRYPTO_WALLET_MANAGER_STATE_CREATED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_MANAGER_STATE_CREATED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_STATE_CONNECTED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_MANAGER_STATE_CONNECTED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_STATE_SYNCING {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_MANAGER_STATE_SYNCING_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_STATE_DELETED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_MANAGER_STATE_DELETED_VALUE;
        }
    };

    private static final int CRYPTO_WALLET_MANAGER_STATE_CREATED_VALUE      = 0;
    private static final int CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED_VALUE = 1;
    private static final int CRYPTO_WALLET_MANAGER_STATE_CONNECTED_VALUE    = 2;
    private static final int CRYPTO_WALLET_MANAGER_STATE_SYNCING_VALUE      = 3;
    private static final int CRYPTO_WALLET_MANAGER_STATE_DELETED_VALUE      = 4;

    public static BRCryptoWalletManagerStateType fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_WALLET_MANAGER_STATE_CREATED_VALUE:      return CRYPTO_WALLET_MANAGER_STATE_CREATED;
            case CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED_VALUE: return CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED;
            case CRYPTO_WALLET_MANAGER_STATE_CONNECTED_VALUE:    return CRYPTO_WALLET_MANAGER_STATE_CONNECTED;
            case CRYPTO_WALLET_MANAGER_STATE_SYNCING_VALUE:      return CRYPTO_WALLET_MANAGER_STATE_SYNCING;
            case CRYPTO_WALLET_MANAGER_STATE_DELETED_VALUE:      return CRYPTO_WALLET_MANAGER_STATE_DELETED;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
