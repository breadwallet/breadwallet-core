/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.support.UInt256;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;
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

    static CoreBRCryptoAmount createAsEth(UInt256.ByValue value, CoreBRCryptoCurrency currency) {
        BRCryptoCurrency currencyCore = currency.asBRCryptoCurrency();
        BRCryptoAmount amountCore = CryptoLibrary.INSTANCE.cryptoAmountCreate(currencyCore, BRCryptoBoolean.CRYPTO_FALSE, value);
        return new OwnedBRCryptoAmount(amountCore);
    }

    static CoreBRCryptoAmount createAsBtc(UnsignedLong value, CoreBRCryptoCurrency currency) {
        UInt256.ByValue valueBytes = CryptoLibrary.INSTANCE.createUInt256(value.longValue());
        BRCryptoCurrency currencyCore = currency.asBRCryptoCurrency();
        BRCryptoAmount amountCore = CryptoLibrary.INSTANCE.cryptoAmountCreate(currencyCore, BRCryptoBoolean.CRYPTO_FALSE, valueBytes);
        return new OwnedBRCryptoAmount(amountCore);
    }

    CoreBRCryptoCurrency getCurrency();

    Optional<Double> getDouble(CoreBRCryptoUnit unit);

    Optional<UnsignedLong> getIntegerRaw();

    CoreBRCryptoAmount add(CoreBRCryptoAmount amount);

    CoreBRCryptoAmount sub(CoreBRCryptoAmount amount);

    CoreBRCryptoAmount negate();

    boolean isNegative();

    int compare(CoreBRCryptoAmount o);

    boolean isCompatible(CoreBRCryptoAmount o);

    String toStringWithBase(int base, String preface);

    BRCryptoAmount asBRCryptoAmount();
}
