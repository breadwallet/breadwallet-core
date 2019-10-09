/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.google.common.collect.ImmutableMap;

import static com.google.common.base.Preconditions.checkState;

public enum BRCryptoStatus {

    CRYPTO_SUCCESS {
        @Override
        public int toNative() {
            return 0;
        }
    },

    CRYPTO_ERROR_FAILED {
        @Override
        public int toNative() {
            return 1;
        }
    },

    // Reference access
    CRYPTO_ERROR_UNKNOWN_NODE {
        @Override
        public int toNative() {
            return 10000;
        }
    },
    CRYPTO_ERROR_UNKNOWN_TRANSFER {
        @Override
        public int toNative() {
            return 10001;
        }
    },
    CRYPTO_ERROR_UNKNOWN_ACCOUNT {
        @Override
        public int toNative() {
            return 10002;
        }
    },
    CRYPTO_ERROR_UNKNOWN_WALLET {
        @Override
        public int toNative() {
            return 10003;
        }
    },
    CRYPTO_ERROR_UNKNOWN_BLOCK {
        @Override
        public int toNative() {
            return 10004;
        }
    },
    CRYPTO_ERROR_UNKNOWN_LISTENER {
        @Override
        public int toNative() {
            return 10005;
        }
    },

    // Node
    CRYPTO_ERROR_NODE_NOT_CONNECTED {
        @Override
        public int toNative() {
            return 20000;
        }
    },

    // Transfer
    CRYPTO_ERROR_TRANSFER_HASH_MISMATCH {
        @Override
        public int toNative() {
            return 30000;
        }
    },
    CRYPTO_ERROR_TRANSFER_SUBMISSION {
        @Override
        public int toNative() {
            return 30001;
        }
    },

    // Numeric
    CRYPTO_ERROR_NUMERIC_PARSE {
        @Override
        public int toNative() {
            return 40000;
        }
    };

    private static final ImmutableMap<Integer, BRCryptoStatus> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRCryptoStatus> b = ImmutableMap.builder();

        b.put(CRYPTO_SUCCESS.toNative(),                      CRYPTO_SUCCESS);
        b.put(CRYPTO_ERROR_FAILED.toNative(),                 CRYPTO_ERROR_FAILED);

        b.put(CRYPTO_ERROR_UNKNOWN_NODE.toNative(),           CRYPTO_ERROR_UNKNOWN_NODE);
        b.put(CRYPTO_ERROR_UNKNOWN_TRANSFER.toNative(),       CRYPTO_ERROR_UNKNOWN_TRANSFER);
        b.put(CRYPTO_ERROR_UNKNOWN_ACCOUNT.toNative(),        CRYPTO_ERROR_UNKNOWN_ACCOUNT);
        b.put(CRYPTO_ERROR_UNKNOWN_WALLET.toNative(),         CRYPTO_ERROR_UNKNOWN_WALLET);
        b.put(CRYPTO_ERROR_UNKNOWN_BLOCK.toNative(),          CRYPTO_ERROR_UNKNOWN_BLOCK);
        b.put(CRYPTO_ERROR_UNKNOWN_LISTENER.toNative(),       CRYPTO_ERROR_UNKNOWN_LISTENER);

        b.put(CRYPTO_ERROR_NODE_NOT_CONNECTED.toNative(),     CRYPTO_ERROR_NODE_NOT_CONNECTED);

        b.put(CRYPTO_ERROR_TRANSFER_HASH_MISMATCH.toNative(), CRYPTO_ERROR_TRANSFER_HASH_MISMATCH);
        b.put(CRYPTO_ERROR_TRANSFER_SUBMISSION.toNative(),    CRYPTO_ERROR_TRANSFER_SUBMISSION);

        b.put(CRYPTO_ERROR_NUMERIC_PARSE.toNative(),          CRYPTO_ERROR_NUMERIC_PARSE);

        LOOKUP = b.build();
    }

    public static BRCryptoStatus fromNative(int nativeValue) {
        BRCryptoStatus status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
