package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoTransferAttribute extends PointerType {
    public BRCryptoTransferAttribute() {
        super();
    }

    public BRCryptoTransferAttribute(Pointer address) {
        super(address);
    }

    public String getKey() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoTransferAttributeGetKey(thisPtr).getString(0, "UTF-8");
    }

    public String getValue() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoTransferAttributeGetValue(thisPtr).getString(0, "UTF-8");
    }

    public void setValue(String value) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoTransferAttributeSetValue(thisPtr, value);
    }

    public boolean isRequired () {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoTransferAttributeIsRequired(thisPtr);
    }

    public BRCryptoTransferAttribute copy () {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoTransferAttribute(CryptoLibraryDirect.cryptoTransferAttributeCopy (thisPtr));
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoTransferAttributeGive(thisPtr);
    }

}
