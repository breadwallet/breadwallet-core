package com.breadwallet.crypto.jni.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.support.BRMasterPubKey;

public interface CoreBRCryptoAccount {

    static CoreBRCryptoAccount create(String phrase) {
        return new OwnedBRCryptoAccount(CryptoLibrary.INSTANCE.cryptoAccountCreate(phrase));
    }

    static CoreBRCryptoAccount createFromSeed(byte[] seed) {
        return new OwnedBRCryptoAccount(CryptoLibrary.INSTANCE.cryptoAccountCreateFromSeedBytes(seed));
    }

    static byte[] deriveSeed(String phrase) {
        return CryptoLibrary.INSTANCE.cryptoAccountDeriveSeed(phrase).u8.clone();
    }

    long getTimestamp();

    void setTimestamp(long timestamp);

    BRMasterPubKey.ByValue asBtc();

    BRCryptoAccount asBRCryptoAccount();
}
