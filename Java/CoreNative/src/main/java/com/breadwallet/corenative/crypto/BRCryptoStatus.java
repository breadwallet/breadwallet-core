/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

public enum BRCryptoStatus {

    CRYPTO_SUCCESS {
        @Override
        public int toCore() {
            return CRYPTO_SUCCESS_VALUE;
        }
    },
    CRYPTO_ERROR_FAILED {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_FAILED_VALUE;
        }
    },

    // Reference access
    CRYPTO_ERROR_UNKNOWN_NODE {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_UNKNOWN_NODE_VALUE;
        }
    },
    CRYPTO_ERROR_UNKNOWN_TRANSFER {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_UNKNOWN_TRANSFER_VALUE;
        }
    },
    CRYPTO_ERROR_UNKNOWN_ACCOUNT {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_UNKNOWN_ACCOUNT_VALUE;
        }
    },
    CRYPTO_ERROR_UNKNOWN_WALLET {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_UNKNOWN_WALLET_VALUE;
        }
    },
    CRYPTO_ERROR_UNKNOWN_BLOCK {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_UNKNOWN_BLOCK_VALUE;
        }
    },
    CRYPTO_ERROR_UNKNOWN_LISTENER {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_UNKNOWN_LISTENER_VALUE;
        }
    },

    // Node
    CRYPTO_ERROR_NODE_NOT_CONNECTED {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_NODE_NOT_CONNECTED_VALUE;
        }
    },

    // Transfer
    CRYPTO_ERROR_TRANSFER_HASH_MISMATCH {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_TRANSFER_HASH_MISMATCH_VALUE;
        }
    },
    CRYPTO_ERROR_TRANSFER_SUBMISSION {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_TRANSFER_SUBMISSION_VALUE;
        }
    },

    // Numeric
    CRYPTO_ERROR_NUMERIC_PARSE {
        @Override
        public int toCore() {
            return CRYPTO_ERROR_NUMERIC_PARSE_VALUE;
        }
    };

    private static final int CRYPTO_SUCCESS_VALUE                       = 0;
    private static final int CRYPTO_ERROR_FAILED_VALUE                  = 1;

    private static final int CRYPTO_ERROR_UNKNOWN_NODE_VALUE            = 10000;
    private static final int CRYPTO_ERROR_UNKNOWN_TRANSFER_VALUE        = 10001;
    private static final int CRYPTO_ERROR_UNKNOWN_ACCOUNT_VALUE         = 10002;
    private static final int CRYPTO_ERROR_UNKNOWN_WALLET_VALUE          = 10003;
    private static final int CRYPTO_ERROR_UNKNOWN_BLOCK_VALUE           = 10004;
    private static final int CRYPTO_ERROR_UNKNOWN_LISTENER_VALUE        = 10005;

    private static final int CRYPTO_ERROR_NODE_NOT_CONNECTED_VALUE      = 20000;

    private static final int CRYPTO_ERROR_TRANSFER_HASH_MISMATCH_VALUE  = 30000;
    private static final int CRYPTO_ERROR_TRANSFER_SUBMISSION_VALUE     = 30001;

    private static final int CRYPTO_ERROR_NUMERIC_PARSE_VALUE           = 40000;

    public static BRCryptoStatus fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_SUCCESS_VALUE:                      return CRYPTO_SUCCESS;
            case CRYPTO_ERROR_FAILED_VALUE:                 return CRYPTO_ERROR_FAILED;

            case CRYPTO_ERROR_UNKNOWN_NODE_VALUE:           return CRYPTO_ERROR_UNKNOWN_NODE;
            case CRYPTO_ERROR_UNKNOWN_TRANSFER_VALUE:       return CRYPTO_ERROR_UNKNOWN_TRANSFER;
            case CRYPTO_ERROR_UNKNOWN_ACCOUNT_VALUE:        return CRYPTO_ERROR_UNKNOWN_ACCOUNT;
            case CRYPTO_ERROR_UNKNOWN_WALLET_VALUE:         return CRYPTO_ERROR_UNKNOWN_WALLET;
            case CRYPTO_ERROR_UNKNOWN_BLOCK_VALUE:          return CRYPTO_ERROR_UNKNOWN_BLOCK;
            case CRYPTO_ERROR_UNKNOWN_LISTENER_VALUE:       return CRYPTO_ERROR_UNKNOWN_LISTENER;

            case CRYPTO_ERROR_NODE_NOT_CONNECTED_VALUE:     return CRYPTO_ERROR_NODE_NOT_CONNECTED;

            case CRYPTO_ERROR_TRANSFER_HASH_MISMATCH_VALUE: return CRYPTO_ERROR_TRANSFER_HASH_MISMATCH;
            case CRYPTO_ERROR_TRANSFER_SUBMISSION_VALUE:    return CRYPTO_ERROR_TRANSFER_SUBMISSION;

            case CRYPTO_ERROR_NUMERIC_PARSE_VALUE:          return CRYPTO_ERROR_NUMERIC_PARSE;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
