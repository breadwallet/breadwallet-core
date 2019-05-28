package com.breadwallet.crypto.jni.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoBoolean;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoAddress extends PointerType implements CoreBRCryptoAddress {

    public BRCryptoAddress(Pointer address) {
        super(address);
    }

    public BRCryptoAddress() {
        super();
    }

    public boolean isIdentical(CoreBRCryptoAddress o) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAddressIsIdentical(this, o.asBRCryptoAddress());
    }

    @Override
    public BRCryptoAddress asBRCryptoAddress() {
        return this;
    }

    @Override
    public String toString() {
        Pointer addressPtr = CryptoLibrary.INSTANCE.cryptoAddressAsString(this);
        String addressStr = addressPtr.getString(0, "UTF-8");
        Native.free(Pointer.nativeValue(addressPtr));
        return addressStr;
    }
}
