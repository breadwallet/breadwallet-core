/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoWalletManagerEventType {

    CRYPTO_WALLET_MANAGER_EVENT_CREATED {
        @Override
        public int toNative() {
            return CRYPTO_WALLET_MANAGER_EVENT_CREATED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_EVENT_CHANGED {
        @Override
        public int toNative() {
            return CRYPTO_WALLET_MANAGER_EVENT_CHANGED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_EVENT_DELETED {
        @Override
        public int toNative() {
            return CRYPTO_WALLET_MANAGER_EVENT_DELETED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED {
        @Override
        public int toNative() {
            return CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED {
        @Override
        public int toNative() {
            return CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED {
        @Override
        public int toNative() {
            return CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED {
        @Override
        public int toNative() {
            return CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES {
        @Override
        public int toNative() {
            return CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED {
        @Override
        public int toNative() {
            return CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED_VALUE;
        }
    },

    CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED {
        @Override
        public int toNative() {
            return CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED_VALUE;
        }
    };

    private static final int CRYPTO_WALLET_MANAGER_EVENT_CREATED_VALUE              = 0;
    private static final int CRYPTO_WALLET_MANAGER_EVENT_CHANGED_VALUE              = 1;
    private static final int CRYPTO_WALLET_MANAGER_EVENT_DELETED_VALUE              = 2;
    private static final int CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED_VALUE         = 3;
    private static final int CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED_VALUE       = 4;
    private static final int CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED_VALUE       = 5;
    private static final int CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED_VALUE         = 6;
    private static final int CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES_VALUE       = 7;
    private static final int CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED_VALUE         = 8;
    private static final int CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED_VALUE = 9;

    public static BRCryptoWalletManagerEventType fromNative(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_WALLET_MANAGER_EVENT_CREATED_VALUE:              return CRYPTO_WALLET_MANAGER_EVENT_CREATED;
            case CRYPTO_WALLET_MANAGER_EVENT_CHANGED_VALUE:              return CRYPTO_WALLET_MANAGER_EVENT_CHANGED;
            case CRYPTO_WALLET_MANAGER_EVENT_DELETED_VALUE:              return CRYPTO_WALLET_MANAGER_EVENT_DELETED;
            case CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED_VALUE:         return CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED;
            case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED_VALUE:       return CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED;
            case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED_VALUE:       return CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED;
            case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED_VALUE:         return CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED;
            case CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES_VALUE:       return CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES;
            case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED_VALUE:         return CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED;
            case CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED_VALUE: return CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED;
            default: throw new IllegalArgumentException("Invalid native value");
        }
    }

    public abstract int toNative();
}
