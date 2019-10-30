/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/29/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoPaymentProtocolType {

    CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY {
        @Override
        public int toCore() {
            return CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY_VALUE;
        }
    },

    CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70 {
        @Override
        public int toCore() {
            return CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70_VALUE;
        }
    };

    private static final int CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY_VALUE  = 0;
    private static final int CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70_VALUE   = 1;

    public static BRCryptoPaymentProtocolType fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY_VALUE: return CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY;
            case CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70_VALUE:  return CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
