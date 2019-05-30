package com.breadwallet.crypto.jni.bitcoin;


import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.support.UInt256;

/* package */
class OwnedBRTransaction implements CoreBRTransaction {

    private final BRTransaction core;

    /* package */
    OwnedBRTransaction(BRTransaction core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.BRTransactionFree(core);
        }
    }

    @Override
    public BRTxInput[] getInputs() {
        return core.getInputs();
    }

    @Override
    public BRTxOutput[] getOutputs() {
        return core.getOutputs();
    }

    @Override
    public UInt256 getTxHash() {
        return core.getTxHash();
    }

    @Override
    public BRTransaction asBRTransaction() {
        return core;
    }

    @Override
    public BRTransaction asBRTransactionDeepCopy() {
        return core.asBRTransactionDeepCopy();
    }

    @Override
    public byte[] serialize() {
        return core.serialize();
    }
}
