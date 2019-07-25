package com.breadwallet.crypto;

public enum  AddressScheme {
    BTC_LEGACY,
    BTC_SEGWIT,
    ETH_DEFAULT,
    GEN_DEFAULT;

    @Override
    public String toString() {
        switch (this) {
            case BTC_LEGACY:
                return "BTC Legacy";
            case BTC_SEGWIT:
                return "BTC Segwit";
            default:
                return "Default";
        }
    }
}
