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
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.ptr.IntByReference;

import static com.google.common.base.Preconditions.checkArgument;

public class BRCryptoAmount extends PointerType {

    public static BRCryptoAmount create(double value, CoreBRCryptoUnit unit) {
        return CryptoLibrary.INSTANCE.cryptoAmountCreateDouble(value, unit.asBRCryptoUnit());
    }

    public static BRCryptoAmount create(long value, CoreBRCryptoUnit unit) {
        return CryptoLibrary.INSTANCE.cryptoAmountCreateInteger(value, unit.asBRCryptoUnit());
    }

    public static Optional<BRCryptoAmount> create(String value, boolean isNegative, CoreBRCryptoUnit unit) {
        return Optional.fromNullable(
                CryptoLibrary.INSTANCE.cryptoAmountCreateString(
                        value,
                        isNegative ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE,
                        unit.asBRCryptoUnit())
        );
    }

    public static BRCryptoAmount createOwned(BRCryptoAmount amount) {
        // TODO(fix): Can the use case here (called when parsed out of struct) be replaced by changing struct to
        //            have BRCryptoAmount.OwnedBRCryptoAmount as its field, instead of BRCryptoAmount?
        return new OwnedBRCryptoAmount(amount.getPointer());
    }

    public BRCryptoAmount(Pointer address) {
        super(address);
    }

    public BRCryptoAmount() {
        super();
    }

    public CoreBRCryptoCurrency getCurrency() {
        return new OwnedBRCryptoCurrency(CryptoLibrary.INSTANCE.cryptoAmountGetCurrency(this));
    }

    public CoreBRCryptoUnit getUnit() {
        return new OwnedBRCryptoUnit(CryptoLibrary.INSTANCE.cryptoAmountGetUnit(this));
    }

    public Optional<Double> getDouble(CoreBRCryptoUnit unit) {
        BRCryptoUnit unitCore = unit.asBRCryptoUnit();
        IntByReference overflowRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        double value = CryptoLibrary.INSTANCE.cryptoAmountGetDouble(this, unitCore, overflowRef);
        return overflowRef.getValue() == BRCryptoBoolean.CRYPTO_TRUE ? Optional.absent() : Optional.of(value);
    }

    public Optional<BRCryptoAmount> add(BRCryptoAmount o) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoAmountAdd(this, o));
    }

    public Optional<BRCryptoAmount> sub(BRCryptoAmount o) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoAmountSub(this, o));
    }

    public BRCryptoAmount negate() {
        return CryptoLibrary.INSTANCE.cryptoAmountNegate(this);
    }

    public Optional<BRCryptoAmount> convert(CoreBRCryptoUnit toUnit) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoAmountConvertToUnit(this, toUnit.asBRCryptoUnit()));
    }

    public boolean isNegative() {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAmountIsNegative(this);
    }

    public int compare(BRCryptoAmount o) {
        return CryptoLibrary.INSTANCE.cryptoAmountCompare(this, o);
    }

    public boolean isCompatible(BRCryptoAmount o) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAmountIsCompatible(this, o);
    }

    public boolean hasCurrency(CoreBRCryptoCurrency o) {
        BRCryptoCurrency otherCore = o.asBRCryptoCurrency();
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAmountHasCurrency(this, otherCore);
    }

    public String toStringWithBase(int base, String preface) {
        UInt256.ByValue value = CryptoLibrary.INSTANCE.cryptoAmountGetValue(this);
        Pointer ptr = CryptoLibrary.INSTANCE.coerceStringPrefaced(value, base, preface);
        try {
            return ptr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(ptr));
        }

    }

    public static class OwnedBRCryptoAmount extends BRCryptoAmount {

        public OwnedBRCryptoAmount(Pointer address) {
            super(address);
        }

        public OwnedBRCryptoAmount() {
            super();
        }

        @Override
        protected void finalize() {
            if (null != getPointer()) {
                CryptoLibrary.INSTANCE.cryptoAmountGive(this);
            }
        }
    }
}
