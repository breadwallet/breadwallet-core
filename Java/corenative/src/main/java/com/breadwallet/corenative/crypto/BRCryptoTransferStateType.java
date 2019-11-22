/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoTransferStateType {

    CRYPTO_TRANSFER_STATE_CREATED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_STATE_CREATED_VALUE;
        }
    },

    CRYPTO_TRANSFER_STATE_SIGNED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_STATE_SIGNED_VALUE;
        }
    },

    CRYPTO_TRANSFER_STATE_SUBMITTED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_STATE_SUBMITTED_VALUE;
        }
    },

    CRYPTO_TRANSFER_STATE_INCLUDED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_STATE_INCLUDED_VALUE;
        }
    },

    CRYPTO_TRANSFER_STATE_ERRORED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_STATE_ERRORED_VALUE;
        }
    },

    CRYPTO_TRANSFER_STATE_DELETED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_STATE_DELETED_VALUE;
        }
    };

    private static final int CRYPTO_TRANSFER_STATE_CREATED_VALUE   = 0;
    private static final int CRYPTO_TRANSFER_STATE_SIGNED_VALUE    = 1;
    private static final int CRYPTO_TRANSFER_STATE_SUBMITTED_VALUE = 2;
    private static final int CRYPTO_TRANSFER_STATE_INCLUDED_VALUE  = 3;
    private static final int CRYPTO_TRANSFER_STATE_ERRORED_VALUE   = 4;
    private static final int CRYPTO_TRANSFER_STATE_DELETED_VALUE   = 5;

    public static BRCryptoTransferStateType fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_TRANSFER_STATE_CREATED_VALUE:   return CRYPTO_TRANSFER_STATE_CREATED;
            case CRYPTO_TRANSFER_STATE_SIGNED_VALUE:    return CRYPTO_TRANSFER_STATE_SIGNED;
            case CRYPTO_TRANSFER_STATE_SUBMITTED_VALUE: return CRYPTO_TRANSFER_STATE_SUBMITTED;
            case CRYPTO_TRANSFER_STATE_INCLUDED_VALUE:  return CRYPTO_TRANSFER_STATE_INCLUDED;
            case CRYPTO_TRANSFER_STATE_ERRORED_VALUE:   return CRYPTO_TRANSFER_STATE_ERRORED;
            case CRYPTO_TRANSFER_STATE_DELETED_VALUE:   return CRYPTO_TRANSFER_STATE_DELETED;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
