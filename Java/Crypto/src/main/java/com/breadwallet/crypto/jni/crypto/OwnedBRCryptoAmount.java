package com.breadwallet.crypto.jni.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.sun.jna.ptr.IntByReference;

/* package */
class OwnedBRCryptoAmount implements CoreBRCryptoAmount {

    private final BRCryptoAmount core;

    /* package */
    OwnedBRCryptoAmount(BRCryptoAmount core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoAmountGive(core);
        }
    }

    @Override
    public CoreBRCryptoCurrency getCurrency() {
        return core.getCurrency();
    }

    @Override
    public double getDouble(CoreBRCryptoUnit unit, IntByReference overflow) {
        return core.getDouble(unit, overflow);
    }

    @Override
    public long getIntegerRaw(IntByReference overflow) {
        return core.getIntegerRaw(overflow);
    }

    @Override
    public CoreBRCryptoAmount add(CoreBRCryptoAmount amount) {
        return new OwnedBRCryptoAmount(core.add(amount));
    }

    @Override
    public CoreBRCryptoAmount sub(CoreBRCryptoAmount amount) {
        return new OwnedBRCryptoAmount(core.sub(amount));
    }

    @Override
    public CoreBRCryptoAmount negate() {
        return new OwnedBRCryptoAmount(core.negate());
    }

    @Override
    public boolean isNegative() {
        return core.isNegative();
    }

    @Override
    public int compare(CoreBRCryptoAmount o) {
        return core.compare(o);
    }

    @Override
    public boolean isCompatible(CoreBRCryptoAmount o) {
        return core.isCompatible(o);
    }

    @Override
    public String toStringWithBase(int base, String preface) {
        return core.toStringWithBase(base, preface);
    }

    @Override
    public BRCryptoAmount asBRCryptoAmount() {
        return core;
    }
}
