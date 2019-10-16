/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoTransferEventType {

    CRYPTO_TRANSFER_EVENT_CREATED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_EVENT_CREATED_VALUE;
        }
    },

    CRYPTO_TRANSFER_EVENT_CHANGED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_EVENT_CHANGED_VALUE;
        }
    },

    CRYPTO_TRANSFER_EVENT_DELETED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_EVENT_DELETED_VALUE;
        }
    };

    private static final int CRYPTO_TRANSFER_EVENT_CREATED_VALUE = 0;
    private static final int CRYPTO_TRANSFER_EVENT_CHANGED_VALUE = 1;
    private static final int CRYPTO_TRANSFER_EVENT_DELETED_VALUE = 2;

    public static BRCryptoTransferEventType fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_TRANSFER_EVENT_CREATED_VALUE: return CRYPTO_TRANSFER_EVENT_CREATED;
            case CRYPTO_TRANSFER_EVENT_CHANGED_VALUE: return CRYPTO_TRANSFER_EVENT_CHANGED;
            case CRYPTO_TRANSFER_EVENT_DELETED_VALUE: return CRYPTO_TRANSFER_EVENT_DELETED;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
