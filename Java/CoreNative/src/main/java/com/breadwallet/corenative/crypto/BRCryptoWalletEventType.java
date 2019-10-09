/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoWalletEventType {

    CRYPTO_WALLET_EVENT_CREATED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_EVENT_CREATED_VALUE;
        }
    },

    CRYPTO_WALLET_EVENT_CHANGED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_EVENT_CHANGED_VALUE;
        }
    },

    CRYPTO_WALLET_EVENT_DELETED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_EVENT_DELETED_VALUE;
        }
    },

    CRYPTO_WALLET_EVENT_TRANSFER_ADDED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_EVENT_TRANSFER_ADDED_VALUE;
        }
    },

    CRYPTO_WALLET_EVENT_TRANSFER_CHANGED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_EVENT_TRANSFER_CHANGED_VALUE;
        }
    },

    CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED_VALUE;
        }
    },

    CRYPTO_WALLET_EVENT_TRANSFER_DELETED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_EVENT_TRANSFER_DELETED_VALUE;
        }
    },

    CRYPTO_WALLET_EVENT_BALANCE_UPDATED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_EVENT_BALANCE_UPDATED_VALUE;
        }
    },

    CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED_VALUE;
        }
    },

    CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED_VALUE;
        }
    };

    private static final int CRYPTO_WALLET_EVENT_CREATED_VALUE              = 0;
    private static final int CRYPTO_WALLET_EVENT_CHANGED_VALUE              = 1;
    private static final int CRYPTO_WALLET_EVENT_DELETED_VALUE              = 2;
    private static final int CRYPTO_WALLET_EVENT_TRANSFER_ADDED_VALUE       = 3;
    private static final int CRYPTO_WALLET_EVENT_TRANSFER_CHANGED_VALUE     = 4;
    private static final int CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED_VALUE   = 5;
    private static final int CRYPTO_WALLET_EVENT_TRANSFER_DELETED_VALUE     = 6;
    private static final int CRYPTO_WALLET_EVENT_BALANCE_UPDATED_VALUE      = 7;
    private static final int CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED_VALUE    = 8;
    private static final int CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED_VALUE  = 9;

    public static BRCryptoWalletEventType fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_WALLET_EVENT_CREATED_VALUE:             return CRYPTO_WALLET_EVENT_CREATED;
            case CRYPTO_WALLET_EVENT_CHANGED_VALUE:             return CRYPTO_WALLET_EVENT_CHANGED;
            case CRYPTO_WALLET_EVENT_DELETED_VALUE:             return CRYPTO_WALLET_EVENT_DELETED;
            case CRYPTO_WALLET_EVENT_TRANSFER_ADDED_VALUE:      return CRYPTO_WALLET_EVENT_TRANSFER_ADDED;
            case CRYPTO_WALLET_EVENT_TRANSFER_CHANGED_VALUE:    return CRYPTO_WALLET_EVENT_TRANSFER_CHANGED;
            case CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED_VALUE:  return CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED;
            case CRYPTO_WALLET_EVENT_TRANSFER_DELETED_VALUE:    return CRYPTO_WALLET_EVENT_TRANSFER_DELETED;
            case CRYPTO_WALLET_EVENT_BALANCE_UPDATED_VALUE:     return CRYPTO_WALLET_EVENT_BALANCE_UPDATED;
            case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED_VALUE:   return CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED;
            case CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED_VALUE: return CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
