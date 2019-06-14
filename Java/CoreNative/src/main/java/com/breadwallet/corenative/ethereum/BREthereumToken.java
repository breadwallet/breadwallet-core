package com.breadwallet.corenative.ethereum;

import com.breadwallet.corenative.CryptoLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BREthereumToken extends PointerType {

    public BREthereumToken(Pointer address) {
        super(address);
    }

    public BREthereumToken() {
        super();
    }

    public String getAddress() {
        return CryptoLibrary.INSTANCE.tokenGetAddress(this).getString(0, "UTF-8");
    }
}
