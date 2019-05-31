package com.breadwallet.crypto.jni.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.google.common.primitives.UnsignedBytes;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoUnit extends PointerType implements CoreBRCryptoUnit {

    public BRCryptoUnit(Pointer address) {
        super(address);
    }

    public BRCryptoUnit() {
        super();
    }

    @Override
    public BRCryptoUnit asBRCryptoUnit() {
        return this;
    }


    @Override
    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetName(this);
    }

    @Override
    public String getSymbol() {
        return CryptoLibrary.INSTANCE.cryptoUnitGetSymbol(this);
    }

    @Override
    public int getDecimals() {
        return UnsignedBytes.toInt(CryptoLibrary.INSTANCE.cryptoUnitGetBaseDecimalOffset(this));
    }

    @Override
    public boolean isCompatible(CoreBRCryptoUnit other) {
        BRCryptoUnit otherCore = other.asBRCryptoUnit();
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoUnitIsCompatible(this, otherCore);
    }

    @Override
    public boolean hasCurrency(CoreBRCryptoCurrency currency) {
        BRCryptoCurrency coreCurrency = currency.asBRCryptoCurrency();
        return coreCurrency.equals(CryptoLibrary.INSTANCE.cryptoUnitGetCurrency(this));
    }
}
