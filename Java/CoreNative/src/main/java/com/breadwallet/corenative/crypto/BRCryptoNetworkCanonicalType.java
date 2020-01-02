package com.breadwallet.corenative.crypto;

public enum BRCryptoNetworkCanonicalType {

    CRYPTO_NETWORK_TYPE_BTC {
        @Override
        public int toCore() {
            return CRYPTO_NETWORK_TYPE_BTC_VALUE;
        }
    },

    CRYPTO_NETWORK_TYPE_BCH {
        @Override
        public int toCore() {
            return CRYPTO_NETWORK_TYPE_BCH_VALUE;
        }
    },

    CRYPTO_NETWORK_TYPE_ETH {
        @Override
        public int toCore() {
            return CRYPTO_NETWORK_TYPE_ETH_VALUE;
        }
    },

    CRYPTO_NETWORK_TYPE_XRP {
        @Override
        public int toCore() {
            return CRYPTO_NETWORK_TYPE_XRP_VALUE;
        }
    };

    private static final int CRYPTO_NETWORK_TYPE_BTC_VALUE = 0;
    private static final int CRYPTO_NETWORK_TYPE_BCH_VALUE = 1;
    private static final int CRYPTO_NETWORK_TYPE_ETH_VALUE = 2;
    private static final int CRYPTO_NETWORK_TYPE_XRP_VALUE = 3;

    public static BRCryptoNetworkCanonicalType fromCore(int nativeValue) {
        switch (nativeValue) {
            case CRYPTO_NETWORK_TYPE_BTC_VALUE: return CRYPTO_NETWORK_TYPE_BTC;
            case CRYPTO_NETWORK_TYPE_BCH_VALUE: return CRYPTO_NETWORK_TYPE_BCH;
            case CRYPTO_NETWORK_TYPE_ETH_VALUE: return CRYPTO_NETWORK_TYPE_ETH;
            case CRYPTO_NETWORK_TYPE_XRP_VALUE: return CRYPTO_NETWORK_TYPE_XRP;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
