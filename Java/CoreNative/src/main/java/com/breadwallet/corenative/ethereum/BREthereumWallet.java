package com.breadwallet.corenative.ethereum;

import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BREthereumWallet extends PointerType {

    public BREthereumWallet(Pointer address) {
        super(address);
    }

    public BREthereumWallet() {
        super();
    }
}
