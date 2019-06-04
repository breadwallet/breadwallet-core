package com.breadwallet.crypto.libcrypto.bitcoin;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRPeerManager extends PointerType {

    public interface BRPeerManagerPublishTxCallback extends Callback {
        void apply(Pointer info, int error);
    }

    public BRPeerManager(Pointer address) {
        super(address);
    }

    public BRPeerManager() {
        super();
    }

    public void publishTransaction(CoreBRTransaction transaction, BRPeerManagerPublishTxCallback publishTxCallback) {
        // TODO(discuss): We copy here so that we don't have our memory free'd from underneath us; is this OK? Or should we be using BRWalletManagerSubmitTransaction?
        BRTransaction transactionCopy = transaction.asBRTransactionDeepCopy();
        CryptoLibrary.INSTANCE.BRPeerManagerPublishTx(this, transactionCopy, null, publishTxCallback);
    }
}
