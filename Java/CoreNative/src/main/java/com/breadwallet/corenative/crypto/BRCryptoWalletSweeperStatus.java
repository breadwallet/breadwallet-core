/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoWalletSweeperStatus {

    CRYPTO_WALLET_SWEEPER_SUCCESS {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_SWEEPER_SUCCESS_VALUE;
        }
    },

    CRYPTO_WALLET_SWEEPER_UNSUPPORTED_CURRENCY {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_SWEEPER_UNSUPPORTED_CURRENCY_VALUE;
        }
    },

    CRYPTO_WALLET_SWEEPER_INVALID_KEY {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_SWEEPER_INVALID_KEY_VALUE;
        }
    },

    CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS_VALUE;
        }
    },

    CRYPTO_WALLET_SWEEPER_INVALID_TRANSACTION {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_SWEEPER_INVALID_TRANSACTION_VALUE;
        }
    },

    CRYPTO_WALLET_SWEEPER_INVALID_SOURCE_WALLET {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_SWEEPER_INVALID_SOURCE_WALLET_VALUE;
        }
    },

    CRYPTO_WALLET_SWEEPER_NO_TRANSFERS_FOUND {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_SWEEPER_NO_TRANSFERS_FOUND_VALUE;
        }
    },

    CRYPTO_WALLET_SWEEPER_INSUFFICIENT_FUNDS{
        @Override
        public int toCore() {
            return CRYPTO_WALLET_SWEEPER_INSUFFICIENT_FUNDS_VALUE;
        }
    },

    CRYPTO_WALLET_SWEEPER_UNABLE_TO_SWEEP  {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_SWEEPER_UNABLE_TO_SWEEP_VALUE;
        }
    },

    CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION  {
        @Override
        public int toCore() {
            return CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION_VALUE;
        }
    };

    private static final int CRYPTO_WALLET_SWEEPER_SUCCESS_VALUE                = 0;
    private static final int CRYPTO_WALLET_SWEEPER_UNSUPPORTED_CURRENCY_VALUE   = 1;
    private static final int CRYPTO_WALLET_SWEEPER_INVALID_KEY_VALUE            = 2;
    private static final int CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS_VALUE      = 3;
    private static final int CRYPTO_WALLET_SWEEPER_INVALID_TRANSACTION_VALUE    = 4;
    private static final int CRYPTO_WALLET_SWEEPER_INVALID_SOURCE_WALLET_VALUE  = 5;
    private static final int CRYPTO_WALLET_SWEEPER_NO_TRANSFERS_FOUND_VALUE     = 6;
    private static final int CRYPTO_WALLET_SWEEPER_INSUFFICIENT_FUNDS_VALUE     = 7;
    private static final int CRYPTO_WALLET_SWEEPER_UNABLE_TO_SWEEP_VALUE        = 8;
    private static final int CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION_VALUE      = 9;

    public static BRCryptoWalletSweeperStatus fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_WALLET_SWEEPER_SUCCESS_VALUE:               return CRYPTO_WALLET_SWEEPER_SUCCESS;
            case CRYPTO_WALLET_SWEEPER_UNSUPPORTED_CURRENCY_VALUE:  return CRYPTO_WALLET_SWEEPER_UNSUPPORTED_CURRENCY;
            case CRYPTO_WALLET_SWEEPER_INVALID_KEY_VALUE:           return CRYPTO_WALLET_SWEEPER_INVALID_KEY;
            case CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS_VALUE:     return CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS;
            case CRYPTO_WALLET_SWEEPER_INVALID_TRANSACTION_VALUE:   return CRYPTO_WALLET_SWEEPER_INVALID_TRANSACTION;
            case CRYPTO_WALLET_SWEEPER_INVALID_SOURCE_WALLET_VALUE: return CRYPTO_WALLET_SWEEPER_INVALID_SOURCE_WALLET;
            case CRYPTO_WALLET_SWEEPER_NO_TRANSFERS_FOUND_VALUE:    return CRYPTO_WALLET_SWEEPER_NO_TRANSFERS_FOUND;
            case CRYPTO_WALLET_SWEEPER_INSUFFICIENT_FUNDS_VALUE:    return CRYPTO_WALLET_SWEEPER_INSUFFICIENT_FUNDS;
            case CRYPTO_WALLET_SWEEPER_UNABLE_TO_SWEEP_VALUE:       return CRYPTO_WALLET_SWEEPER_UNABLE_TO_SWEEP;
            case CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION_VALUE:     return CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
