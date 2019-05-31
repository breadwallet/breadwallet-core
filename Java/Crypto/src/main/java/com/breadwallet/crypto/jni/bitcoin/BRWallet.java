package com.breadwallet.crypto.jni.bitcoin;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.crypto.BRCryptoBoolean;
import com.breadwallet.crypto.jni.SizeT;
import com.breadwallet.crypto.jni.support.BRAddress;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRWallet extends PointerType {

    public static final long DEFAULT_FEE_PER_KB = (1000L * 10);

    public BRWallet(Pointer address) {
        super(address);
    }

    public BRWallet() {
        super();
    }

    public long getBalance() {
        return CryptoLibrary.INSTANCE.BRWalletBalance(this);
    }

    public long getFeeForTx(CoreBRTransaction tx) {
        BRTransaction coreTransfer = tx.asBRTransaction();
        return CryptoLibrary.INSTANCE.BRWalletFeeForTx(this, coreTransfer);
    }

    public long getFeeForTxAmount(long amount) {
        return CryptoLibrary.INSTANCE.BRWalletFeeForTxAmount(this, amount);
    }

    public long getFeePerKb() {
        return CryptoLibrary.INSTANCE.BRWalletFeePerKb(this);
    }

    public void setFeePerKb(long feePerKb) {
        CryptoLibrary.INSTANCE.BRWalletSetFeePerKb(this, feePerKb);
    }

    public BRAddress.ByValue legacyAddress() {
        return CryptoLibrary.INSTANCE.BRWalletLegacyAddress(this);
    }

    public boolean matches(BRWallet o) {
        return this.equals(o);
    }

    public long getAmountReceivedFromTx(CoreBRTransaction tx) {
        BRTransaction coreTransfer = tx.asBRTransaction();
        return CryptoLibrary.INSTANCE.BRWalletAmountReceivedFromTx(this, coreTransfer);
    }

    public long getAmountSentByTx(CoreBRTransaction tx) {
        BRTransaction coreTransfer = tx.asBRTransaction();
        return CryptoLibrary.INSTANCE.BRWalletAmountSentByTx(this, coreTransfer);
    }

    public boolean containsAddress(String address) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.BRWalletContainsAddress(this, address);
    }

    public boolean signTransaction(CoreBRTransaction tx, byte[] seed) {
        BRTransaction coreTransfer = tx.asBRTransaction();
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.BRWalletSignTransaction(this, coreTransfer, seed, new SizeT(seed.length));
    }
}
