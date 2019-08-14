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
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.ptr.IntByReference;

import static com.google.common.base.Preconditions.checkArgument;

public class BRCryptoAmount extends PointerType implements CoreBRCryptoAmount {

    public BRCryptoAmount(Pointer address) {
        super(address);
    }

    public BRCryptoAmount() {
        super();
    }

    @Override
    public CoreBRCryptoCurrency getCurrency() {
        return new OwnedBRCryptoCurrency(CryptoLibrary.INSTANCE.cryptoAmountGetCurrency(this));
    }

    @Override
    public CoreBRCryptoUnit getUnit() {
        return new OwnedBRCryptoUnit(CryptoLibrary.INSTANCE.cryptoAmountGetUnit(this));
    }

    @Override
    public Optional<Double> getDouble(CoreBRCryptoUnit unit) {
        BRCryptoUnit unitCore = unit.asBRCryptoUnit();
        IntByReference overflowRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        double value = CryptoLibrary.INSTANCE.cryptoAmountGetDouble(this, unitCore, overflowRef);
        return overflowRef.getValue() == BRCryptoBoolean.CRYPTO_TRUE ? Optional.absent() : Optional.of(value);
    }

    @Override
    public Optional<CoreBRCryptoAmount> add(CoreBRCryptoAmount o) {
        BRCryptoAmount otherCore = o.asBRCryptoAmount();
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoAmountAdd(this, otherCore)).transform(OwnedBRCryptoAmount::new);
    }

    @Override
    public Optional<CoreBRCryptoAmount> sub(CoreBRCryptoAmount o) {
        BRCryptoAmount otherCore = o.asBRCryptoAmount();
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoAmountSub(this, otherCore)).transform(OwnedBRCryptoAmount::new);
    }

    @Override
    public CoreBRCryptoAmount negate() {
        return new OwnedBRCryptoAmount(CryptoLibrary.INSTANCE.cryptoAmountNegate(this));
    }

    @Override
    public Optional<CoreBRCryptoAmount> convert(CoreBRCryptoUnit toUnit) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoAmountConvertToUnit(this, toUnit.asBRCryptoUnit())).transform(OwnedBRCryptoAmount::new);
    }

    @Override
    public boolean isNegative() {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAmountIsNegative(this);
    }

    @Override
    public int compare(CoreBRCryptoAmount o) {
        BRCryptoAmount otherCore = o.asBRCryptoAmount();
        return CryptoLibrary.INSTANCE.cryptoAmountCompare(this, otherCore);
    }

    @Override
    public boolean isCompatible(CoreBRCryptoAmount o) {
        BRCryptoAmount otherCore = o.asBRCryptoAmount();
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAmountIsCompatible(this, otherCore);
    }

    @Override
    public boolean hasCurrency(CoreBRCryptoCurrency o) {
        BRCryptoCurrency otherCore = o.asBRCryptoCurrency();
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAmountHasCurrency(this, otherCore);
    }

    @Override
    public String toStringWithBase(int base, String preface) {
        UInt256.ByValue value = CryptoLibrary.INSTANCE.cryptoAmountGetValue(this);
        Pointer ptr = CryptoLibrary.INSTANCE.coerceStringPrefaced(value, base, preface);
        try {
            return ptr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(ptr));
        }

    }

    @Override
    public BRCryptoAmount asBRCryptoAmount() {
        return this;
    }
}
