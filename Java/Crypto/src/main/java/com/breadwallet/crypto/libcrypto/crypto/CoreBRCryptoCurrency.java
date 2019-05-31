package com.breadwallet.crypto.libcrypto.crypto;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;

public interface CoreBRCryptoCurrency {

    static CoreBRCryptoCurrency create(String name, String code, String type) {
        return new OwnedBRCryptoCurrency(CryptoLibrary.INSTANCE.cryptoCurrencyCreate(name, code, type));
    }

    String getName();

    String getCode();

    String getType();

    BRCryptoCurrency asBRCryptoCurrency();
}
