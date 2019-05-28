package com.breadwallet.crypto.jni.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.support.BRMasterPubKey;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoAccount extends PointerType implements CoreBRCryptoAccount {

    public BRCryptoAccount(Pointer address) {
        super(address);
    }

    public BRCryptoAccount() {
        super();
    }

    public long getTimestamp() {
        return CryptoLibrary.INSTANCE.cryptoAccountGetTimestamp(this);
    }

    public void setTimestamp(long timestamp) {
        CryptoLibrary.INSTANCE.cryptoAccountSetTimestamp(this, timestamp);
    }

    @Override
    public BRMasterPubKey.ByValue asBtc() {
        return CryptoLibrary.INSTANCE.cryptoAccountAsBTC(this);
    }

    @Override
    public BRCryptoAccount asBRCryptoAccount() {
        return this;
    }
}
