/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
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

    static CoreBRCryptoAmount create(double value, CoreBRCryptoUnit unit) {
        BRCryptoAmount amount = CryptoLibrary.INSTANCE.cryptoAmountCreateDouble(value, unit.asBRCryptoUnit());
        return new OwnedBRCryptoAmount(amount);
    }

    static CoreBRCryptoAmount create(long value, CoreBRCryptoUnit unit) {
        BRCryptoAmount amount = CryptoLibrary.INSTANCE.cryptoAmountCreateInteger(value, unit.asBRCryptoUnit());
        return new OwnedBRCryptoAmount(amount);
    }

    static Optional<CoreBRCryptoAmount> create(String value, boolean isNegative, CoreBRCryptoUnit unit) {
        int isNegativeEnum = isNegative ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE;
        BRCryptoAmount amount = CryptoLibrary.INSTANCE.cryptoAmountCreateString(value, isNegativeEnum,
                unit.asBRCryptoUnit());
        return Optional.fromNullable(amount).transform(OwnedBRCryptoAmount::new);
    }

    static CoreBRCryptoAmount createOwned(BRCryptoAmount amount) {
        return new OwnedBRCryptoAmount(amount);
    }

    static CoreBRCryptoAmount takeAndCreateOwned(BRCryptoAmount amount) {
        return new OwnedBRCryptoAmount(CryptoLibrary.INSTANCE.cryptoAmountTake(amount));
    }

    CoreBRCryptoCurrency getCurrency();

    CoreBRCryptoUnit getUnit();

    Optional<Double> getDouble(CoreBRCryptoUnit unit);

    Optional<CoreBRCryptoAmount> add(CoreBRCryptoAmount amount);

    Optional<CoreBRCryptoAmount> sub(CoreBRCryptoAmount amount);

    CoreBRCryptoAmount negate();

    Optional<CoreBRCryptoAmount> convert(CoreBRCryptoUnit toUnit);

    boolean isNegative();

    int compare(CoreBRCryptoAmount o);

    boolean isCompatible(CoreBRCryptoAmount o);

    boolean hasCurrency(CoreBRCryptoCurrency o);

    String toStringWithBase(int base, String preface);

    BRCryptoAmount asBRCryptoAmount();
}
