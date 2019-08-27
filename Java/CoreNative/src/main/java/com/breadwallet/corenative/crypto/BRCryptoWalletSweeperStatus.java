package com.breadwallet.corenative.crypto;

import com.google.common.collect.ImmutableMap;

import static com.google.common.base.Preconditions.checkState;

public enum BRCryptoWalletSweeperStatus {

    CRYPTO_WALLET_SWEEPER_SUCCESS {
        @Override
        public int toNative() {
            return 0;
        }
    },

    CRYPTO_WALLET_SWEEPER_UNSUPPORTED_CURRENCY {
        @Override
        public int toNative() {
            return 1;
        }
    },

    CRYPTO_WALLET_SWEEPER_INVALID_KEY {
        @Override
        public int toNative() {
            return 2;
        }
    },

    CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS {
        @Override
        public int toNative() {
            return 3;
        }
    },

    CRYPTO_WALLET_SWEEPER_INVALID_TRANSACTION {
        @Override
        public int toNative() {
            return 4;
        }
    },

    CRYPTO_WALLET_SWEEPER_INVALID_SOURCE_WALLET{
        @Override
        public int toNative() {
            return 5;
        }
    },

    CRYPTO_WALLET_SWEEPER_NO_TRANSFERS_FOUND {
        @Override
        public int toNative() {
            return 6;
        }
    },

    CRYPTO_WALLET_SWEEPER_INSUFFICIENT_FUNDS{
        @Override
        public int toNative() {
            return 7;
        }
    },

    CRYPTO_WALLET_SWEEPER_UNABLE_TO_SWEEP  {
        @Override
        public int toNative() {
            return 8;
        }
    },


    CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION  {
        @Override
        public int toNative() {
            return 9;
        }
    };

    private static final ImmutableMap<Integer, BRCryptoWalletSweeperStatus> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRCryptoWalletSweeperStatus> b = ImmutableMap.builder();

        b.put(CRYPTO_WALLET_SWEEPER_SUCCESS.toNative(),                 CRYPTO_WALLET_SWEEPER_SUCCESS);
        b.put(CRYPTO_WALLET_SWEEPER_UNSUPPORTED_CURRENCY.toNative(),    CRYPTO_WALLET_SWEEPER_UNSUPPORTED_CURRENCY);
        b.put(CRYPTO_WALLET_SWEEPER_INVALID_KEY.toNative(),             CRYPTO_WALLET_SWEEPER_INVALID_KEY);
        b.put(CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS.toNative(),       CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS);
        b.put(CRYPTO_WALLET_SWEEPER_INVALID_TRANSACTION.toNative(),     CRYPTO_WALLET_SWEEPER_INVALID_TRANSACTION);
        b.put(CRYPTO_WALLET_SWEEPER_INVALID_SOURCE_WALLET.toNative(),   CRYPTO_WALLET_SWEEPER_INVALID_SOURCE_WALLET);

        b.put(CRYPTO_WALLET_SWEEPER_NO_TRANSFERS_FOUND.toNative(),      CRYPTO_WALLET_SWEEPER_NO_TRANSFERS_FOUND);
        b.put(CRYPTO_WALLET_SWEEPER_INSUFFICIENT_FUNDS.toNative(),      CRYPTO_WALLET_SWEEPER_INSUFFICIENT_FUNDS);
        b.put(CRYPTO_WALLET_SWEEPER_UNABLE_TO_SWEEP.toNative(),         CRYPTO_WALLET_SWEEPER_UNABLE_TO_SWEEP);
        b.put(CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION.toNative(),       CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION);

        LOOKUP = b.build();
    }

    public static BRCryptoWalletSweeperStatus fromNative(int nativeValue) {
        BRCryptoWalletSweeperStatus status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
