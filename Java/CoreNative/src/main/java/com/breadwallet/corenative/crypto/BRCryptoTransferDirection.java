/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoTransferDirection {

    CRYPTO_TRANSFER_SENT {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_SENT_VALUE;
        }
    },

    CRYPTO_TRANSFER_RECEIVED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_RECEIVED_VALUE;
        }
    },

    CRYPTO_TRANSFER_RECOVERED {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_RECOVERED_VALUE;
        }
    };

    private static final int CRYPTO_TRANSFER_SENT_VALUE      = 0;
    private static final int CRYPTO_TRANSFER_RECEIVED_VALUE  = 1;
    private static final int CRYPTO_TRANSFER_RECOVERED_VALUE = 2;

    public static BRCryptoTransferDirection fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_TRANSFER_SENT_VALUE:      return CRYPTO_TRANSFER_SENT;
            case CRYPTO_TRANSFER_RECEIVED_VALUE:  return CRYPTO_TRANSFER_RECEIVED;
            case CRYPTO_TRANSFER_RECOVERED_VALUE: return CRYPTO_TRANSFER_RECOVERED;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
