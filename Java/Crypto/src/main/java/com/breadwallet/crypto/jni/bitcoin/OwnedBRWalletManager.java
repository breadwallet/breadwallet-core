package com.breadwallet.crypto.jni.bitcoin;

import com.breadwallet.crypto.jni.CryptoLibrary;

/* package */
class OwnedBRWalletManager implements CoreBRWalletManager {

    private final BRWalletManager core;

    /* package */
    OwnedBRWalletManager(BRWalletManager core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.BRWalletManagerFree(core);
        }
    }

    @Override
    public BRWallet getWallet() {
        return core.getWallet();
    }

    @Override
    public void connect() {
        core.connect();
    }

    @Override
    public void disconnect() {
        core.disconnect();
    }

    @Override
    public void scan() {
        core.scan();
    }

    @Override
    public boolean matches(BRWalletManager o) {
        return core.matches(o);
    }
}
