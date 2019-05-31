package com.breadwallet.crypto.jni.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.support.UInt256;
import com.google.common.base.Optional;
import com.sun.jna.ptr.IntByReference;

public interface CoreBRCryptoAmount {

    static Optional<CoreBRCryptoAmount> create(double value, CoreBRCryptoUnit unit) {
        BRCryptoAmount amount = CryptoLibrary.INSTANCE.cryptoAmountCreateDouble(value, unit.asBRCryptoUnit());
        return Optional.fromNullable(amount).transform(OwnedBRCryptoAmount::new);
    }

    static Optional<CoreBRCryptoAmount> create(long value, CoreBRCryptoUnit unit) {
        BRCryptoAmount amount = CryptoLibrary.INSTANCE.cryptoAmountCreateInteger(value, unit.asBRCryptoUnit());
        return Optional.fromNullable(amount).transform(OwnedBRCryptoAmount::new);
    }

    static Optional<CoreBRCryptoAmount> create(String value, boolean isNegative, CoreBRCryptoUnit unit) {
        int isNegativeEnum = isNegative ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE;
        BRCryptoAmount amount = CryptoLibrary.INSTANCE.cryptoAmountCreateString(value, isNegativeEnum,
                unit.asBRCryptoUnit());
        return Optional.fromNullable(amount).transform(OwnedBRCryptoAmount::new);
    }

    static CoreBRCryptoAmount createAsBtc(long value, CoreBRCryptoCurrency currency) {
        UInt256.ByValue valueBytes = CryptoLibrary.INSTANCE.createUInt256(value);
        BRCryptoCurrency currencyCore = currency.asBRCryptoCurrency();
        BRCryptoAmount amountCore = CryptoLibrary.INSTANCE.cryptoAmountCreate(currencyCore, BRCryptoBoolean.CRYPTO_FALSE, valueBytes);
        return new OwnedBRCryptoAmount(amountCore);
    }

    CoreBRCryptoCurrency getCurrency();

    double getDouble(CoreBRCryptoUnit unit, IntByReference overflow);

    long getIntegerRaw(IntByReference overflow);

    CoreBRCryptoAmount add(CoreBRCryptoAmount amount);

    CoreBRCryptoAmount sub(CoreBRCryptoAmount amount);

    CoreBRCryptoAmount negate();

    boolean isNegative();

    int compare(CoreBRCryptoAmount o);

    boolean isCompatible(CoreBRCryptoAmount o);

    String toStringWithBase(int base, String preface);

    BRCryptoAmount asBRCryptoAmount();
}
