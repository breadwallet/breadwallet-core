package com.breadwallet.crypto.jni.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.support.UInt256;
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
    public BRCryptoCurrency getCurrency() {
        return CryptoLibrary.INSTANCE.cryptoAmountGetCurrency(this);
    }

    @Override
    public double getDouble(CoreBRCryptoUnit unit, IntByReference overflowRef) {
        BRCryptoUnit unitCore = unit.asBRCryptoUnit();
        return CryptoLibrary.INSTANCE.cryptoAmountGetDouble(this, unitCore, overflowRef);
    }

    @Override
    public long getIntegerRaw(IntByReference overflow) {
        return CryptoLibrary.INSTANCE.cryptoAmountGetIntegerRaw(this, overflow);
    }

    @Override
    public BRCryptoAmount add(CoreBRCryptoAmount o) {
        BRCryptoAmount otherCore = o.asBRCryptoAmount();
        return CryptoLibrary.INSTANCE.cryptoAmountAdd(this, otherCore);
    }

    @Override
    public BRCryptoAmount sub(CoreBRCryptoAmount o) {
        BRCryptoAmount otherCore = o.asBRCryptoAmount();
        return CryptoLibrary.INSTANCE.cryptoAmountSub(this, otherCore);
    }

    @Override
    public BRCryptoAmount negate() {
        return CryptoLibrary.INSTANCE.cryptoAmountNegate(this);
    }

    @Override
    public int isNegative() {
        return CryptoLibrary.INSTANCE.cryptoAmountIsNegative(this);
    }

    @Override
    public int compare(CoreBRCryptoAmount o) {
        BRCryptoAmount otherCore = o.asBRCryptoAmount();
        return CryptoLibrary.INSTANCE.cryptoAmountCompare(this, otherCore);
    }

    @Override
    public int isCompatible(CoreBRCryptoAmount o) {
        BRCryptoAmount otherCore = o.asBRCryptoAmount();
        return CryptoLibrary.INSTANCE.cryptoAmountIsCompatible(this, otherCore);
    }

    @Override
    public String toStringWithBase(int base, String preface) {
        checkArgument(base > 0, "Invalid base value");
        UInt256.ByValue value = CryptoLibrary.INSTANCE.cryptoAmountGetValue(this);
        Pointer ptr = CryptoLibrary.INSTANCE.coerceStringPrefaced(value, base, preface);
        String str = ptr.getString(0, "UTF-8");
        Native.free(Pointer.nativeValue(ptr));
        return str;
    }

    @Override
    public BRCryptoAmount asBRCryptoAmount() {
        return this;
    }
}
