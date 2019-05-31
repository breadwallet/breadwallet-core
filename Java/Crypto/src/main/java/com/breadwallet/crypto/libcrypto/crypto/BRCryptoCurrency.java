package com.breadwallet.crypto.libcrypto.crypto;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoCurrency extends PointerType implements CoreBRCryptoCurrency {

    public BRCryptoCurrency(Pointer address) {
        super(address);
    }

    public BRCryptoCurrency() {
        super();
    }

    @Override
    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetName(this);
    }

    @Override
    public String getCode() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetCode(this);
    }

    @Override
    public String getType() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetType(this);
    }

    @Override
    public BRCryptoCurrency asBRCryptoCurrency() {
        return this;
    }
}
