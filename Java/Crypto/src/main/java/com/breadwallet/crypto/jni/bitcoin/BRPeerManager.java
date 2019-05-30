package com.breadwallet.crypto.jni.bitcoin;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRPeerManagerPublishTxCallback;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRPeerManager extends PointerType {

    public BRPeerManager(Pointer address) {
        super(address);
    }

    public BRPeerManager() {
        super();
    }

    public void publishTransaction(CoreBRTransaction transaction, BRPeerManagerPublishTxCallback publishTxCallback) {
        // TODO: We copy here so that we don't have our memory free'd from underneath us; is this OK?
        BRTransaction transactionCopy = transaction.asBRTransactionDeepCopy();
        CryptoLibrary.INSTANCE.BRPeerManagerPublishTx(this, transactionCopy, null, publishTxCallback);
    }
}
