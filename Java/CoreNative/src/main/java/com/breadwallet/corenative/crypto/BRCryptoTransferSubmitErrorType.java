/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 9/18/19.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import static com.google.common.base.Preconditions.checkState;

public enum BRCryptoTransferSubmitErrorType {

    CRYPTO_TRANSFER_SUBMIT_ERROR_UNKNOWN {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_SUBMIT_ERROR_UNKNOWN_VALUE;
        }
    },

    CRYPTO_TRANSFER_SUBMIT_ERROR_POSIX {
        @Override
        public int toCore() {
            return CRYPTO_TRANSFER_SUBMIT_ERROR_POSIX_VALUE;
        }
    };

    private static final int CRYPTO_TRANSFER_SUBMIT_ERROR_UNKNOWN_VALUE = 0;
    private static final int CRYPTO_TRANSFER_SUBMIT_ERROR_POSIX_VALUE = 1;

    public static BRCryptoTransferSubmitErrorType fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_TRANSFER_SUBMIT_ERROR_UNKNOWN_VALUE: return CRYPTO_TRANSFER_SUBMIT_ERROR_UNKNOWN;
            case CRYPTO_TRANSFER_SUBMIT_ERROR_POSIX_VALUE:   return CRYPTO_TRANSFER_SUBMIT_ERROR_POSIX;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
