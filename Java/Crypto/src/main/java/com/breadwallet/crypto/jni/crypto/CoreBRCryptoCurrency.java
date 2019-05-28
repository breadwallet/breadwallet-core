package com.breadwallet.crypto.jni.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;

public interface CoreBRCryptoCurrency {

    static CoreBRCryptoCurrency create(String name, String code, String type) {
        return new OwnedBRCryptoCurrency(CryptoLibrary.INSTANCE.cryptoCurrencyCreate(name, code, type));
    }

    String getName();

    String getCode();

    String getType();

    BRCryptoCurrency asBRCryptoCurrency();
}
