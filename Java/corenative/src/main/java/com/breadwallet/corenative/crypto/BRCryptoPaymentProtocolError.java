/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/29/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoPaymentProtocolError {

    CRYPTO_PAYMENT_PROTOCOL_ERROR_NONE {
        @Override
        public int toCore() {
            return CRYPTO_PAYMENT_PROTOCOL_ERROR_NONE_VALUE;
        }
    },

    CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_MISSING {
        @Override
        public int toCore() {
            return CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_MISSING_VALUE;
        }
    },

    CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_NOT_TRUSTED {
        @Override
        public int toCore() {
            return CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_NOT_TRUSTED_VALUE;
        }
    },

    CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED {
        @Override
        public int toCore() {
            return CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED_VALUE;
        }
    },

    CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_VERIFICATION_FAILED {
        @Override
        public int toCore() {
            return CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_VERIFICATION_FAILED_VALUE;
        }
    },

    CRYPTO_PAYMENT_PROTOCOL_ERROR_EXPIRED {
        @Override
        public int toCore() {
            return CRYPTO_PAYMENT_PROTOCOL_ERROR_EXPIRED_VALUE;
        }
    };

    private static final int CRYPTO_PAYMENT_PROTOCOL_ERROR_NONE_VALUE                           = 0;
    private static final int CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_MISSING_VALUE                   = 1;
    private static final int CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_NOT_TRUSTED_VALUE               = 2;
    private static final int CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED_VALUE   = 3;
    private static final int CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_VERIFICATION_FAILED_VALUE  = 4;
    private static final int CRYPTO_PAYMENT_PROTOCOL_ERROR_EXPIRED_VALUE                        = 5;

    public static BRCryptoPaymentProtocolError fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_PAYMENT_PROTOCOL_ERROR_NONE_VALUE:                          return CRYPTO_PAYMENT_PROTOCOL_ERROR_NONE;
            case CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_MISSING_VALUE:                  return CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_MISSING;
            case CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_NOT_TRUSTED_VALUE:              return CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_NOT_TRUSTED;
            case CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED_VALUE:  return CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED;
            case CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_VERIFICATION_FAILED_VALUE: return CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_VERIFICATION_FAILED;
            case CRYPTO_PAYMENT_PROTOCOL_ERROR_EXPIRED_VALUE:                       return CRYPTO_PAYMENT_PROTOCOL_ERROR_EXPIRED;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
