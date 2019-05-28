package com.breadwallet.crypto.jni.bitcoin;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.support.BRAddress;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRWallet extends PointerType {

    public BRWallet(Pointer address) {
        super(address);
    }

    public BRWallet() {
        super();
    }

    public long getBalance() {
        return CryptoLibrary.INSTANCE.BRWalletBalance(this);
    }

    public long getFeeForTx(BRTransaction tx) {
        return CryptoLibrary.INSTANCE.BRWalletFeeForTx(this, tx);
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

    public long getAmountReceivedFromTx(BRTransaction coreTransfer) {
        return CryptoLibrary.INSTANCE.BRWalletAmountReceivedFromTx(this, coreTransfer);
    }

    public long getAmountSentByTx(BRTransaction coreTransfer) {
        return CryptoLibrary.INSTANCE.BRWalletAmountSentByTx(this, coreTransfer);
    }

    public int containsAddress(String address) {
        return CryptoLibrary.INSTANCE.BRWalletContainsAddress(this, address);
    }
}
