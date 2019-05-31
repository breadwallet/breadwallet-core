package com.breadwallet.crypto.libcrypto.crypto;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;

/* package */
class OwnedBRCryptoAddress implements CoreBRCryptoAddress {

    private final BRCryptoAddress core;

    /* package */
    OwnedBRCryptoAddress(BRCryptoAddress core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoAddressGive(core);
        }
    }

    @Override
    public boolean isIdentical(CoreBRCryptoAddress address) {
        return core.isIdentical(address);
    }

    @Override
    public String toString() {
        return core.toString();
    }

    @Override
    public BRCryptoAddress asBRCryptoAddress() {
        return core;
    }
}
