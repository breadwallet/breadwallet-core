package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.CoreBRCryptoFeeBasis;

public class TransferFeeBasis implements com.breadwallet.crypto.TransferFeeBasis {

    /* package */
    static TransferFeeBasis create(CoreBRCryptoFeeBasis core) {
        return new TransferFeeBasis(core);
    }

    /* package */
    static TransferFeeBasis from(com.breadwallet.crypto.TransferFeeBasis feeBasis) {
        if (feeBasis instanceof TransferFeeBasis) {
            return (TransferFeeBasis) feeBasis;
        }
        throw new IllegalArgumentException("Unsupported fee basis instance");
    }

    private final CoreBRCryptoFeeBasis core;

    private TransferFeeBasis(CoreBRCryptoFeeBasis core) {
        this.core = core;
    }

    public CoreBRCryptoFeeBasis getCoreBRFeeBasis() {
        return core;
    }
}
