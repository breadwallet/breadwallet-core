package com.breadwallet.crypto.libcrypto.crypto;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;

/* package */
class OwnedBRCryptoUnit implements CoreBRCryptoUnit {

    private final BRCryptoUnit core;

    /* package */
    OwnedBRCryptoUnit(BRCryptoUnit core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoUnitGive(core);
        }
    }

    @Override
    public String getName() {
        return core.getName();
    }

    @Override
    public String getSymbol() {
        return core.getSymbol();
    }

    @Override
    public int getDecimals() {
        return core.getDecimals();
    }

    @Override
    public boolean isCompatible(CoreBRCryptoUnit other) {
        return core.isCompatible(other);
    }

    @Override
    public boolean hasCurrency(CoreBRCryptoCurrency currency) {
        return core.hasCurrency(currency);
    }

    @Override
    public BRCryptoUnit asBRCryptoUnit() {
        return core;
    }
}
