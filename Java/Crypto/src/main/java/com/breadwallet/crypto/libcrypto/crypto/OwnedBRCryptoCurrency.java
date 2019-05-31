package com.breadwallet.crypto.libcrypto.crypto;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;

/* package */
class OwnedBRCryptoCurrency implements CoreBRCryptoCurrency {

    private final BRCryptoCurrency core;

    /* package */
    OwnedBRCryptoCurrency(BRCryptoCurrency core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoCurrencyGive(core);
        }
    }

    @Override
    public String getName() {
        return core.getName();
    }

    @Override
    public String getCode() {
        return core.getCode();
    }

    @Override
    public String getType() {
        return core.getType();
    }

    @Override
    public BRCryptoCurrency asBRCryptoCurrency() {
        return core;
    }
}
