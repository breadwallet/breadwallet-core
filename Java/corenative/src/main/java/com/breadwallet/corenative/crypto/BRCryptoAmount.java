/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.google.common.base.Optional;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.ptr.IntByReference;

public class BRCryptoAmount extends PointerType {

    public static BRCryptoAmount create(double value, BRCryptoUnit unit) {
        return new BRCryptoAmount(CryptoLibraryDirect.cryptoAmountCreateDouble(value, unit.getPointer()));
    }

    public static BRCryptoAmount create(long value, BRCryptoUnit unit) {
        return new BRCryptoAmount(CryptoLibraryDirect.cryptoAmountCreateInteger(value, unit.getPointer()));
    }

    public static Optional<BRCryptoAmount> create(String value, boolean isNegative, BRCryptoUnit unit) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoAmountCreateString(
                        value,
                        isNegative ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE,
                        unit.getPointer())
        ).transform(BRCryptoAmount::new);
    }

    public BRCryptoAmount() {
        super();
    }

    public BRCryptoAmount(Pointer address) {
        super(address);
    }

    public BRCryptoCurrency getCurrency() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoCurrency(CryptoLibraryDirect.cryptoAmountGetCurrency(thisPtr));
    }

    public BRCryptoUnit getUnit() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoUnit(CryptoLibraryDirect.cryptoAmountGetUnit(thisPtr));
    }

    public Optional<Double> getDouble(BRCryptoUnit unit) {
        Pointer thisPtr = this.getPointer();

        IntByReference overflowRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        double value = CryptoLibraryDirect.cryptoAmountGetDouble(thisPtr, unit.getPointer(), overflowRef);
        return overflowRef.getValue() == BRCryptoBoolean.CRYPTO_TRUE ? Optional.absent() : Optional.of(value);
    }

    public Optional<BRCryptoAmount> add(BRCryptoAmount other) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoAmountAdd(
                        thisPtr,
                        other.getPointer()
                )
        ).transform(BRCryptoAmount::new);
    }

    public Optional<BRCryptoAmount> sub(BRCryptoAmount other) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoAmountSub(
                        thisPtr,
                        other.getPointer()
                )
        ).transform(BRCryptoAmount::new);
    }

    public BRCryptoAmount negate() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoAmount(CryptoLibraryDirect.cryptoAmountNegate(thisPtr));
    }

    public Optional<BRCryptoAmount> convert(BRCryptoUnit toUnit) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoAmountConvertToUnit(
                        thisPtr,
                        toUnit.getPointer()
                )
        ).transform(BRCryptoAmount::new);
    }

    public boolean isNegative() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoAmountIsNegative(thisPtr);
    }

    public boolean isZero() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoAmountIsZero(thisPtr);
    }

    public BRCryptoComparison compare(BRCryptoAmount other) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoComparison.fromCore(CryptoLibraryDirect.cryptoAmountCompare(thisPtr, other.getPointer()));
    }

    public boolean isCompatible(BRCryptoAmount amount) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoAmountIsCompatible(thisPtr, amount.getPointer());
    }

    public boolean hasCurrency(BRCryptoCurrency currency) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoAmountHasCurrency(thisPtr, currency.getPointer());
    }

    public String toStringWithBase(int base, String preface) {
        Pointer thisPtr = this.getPointer();

        Pointer ptr = CryptoLibraryDirect.cryptoAmountGetStringPrefaced(thisPtr, base, preface);
        try {
            return ptr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(ptr));
        }
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoAmountGive(thisPtr);
    }
}
