package com.breadwallet.corenative.ethereum;

import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BREthereumTransfer extends PointerType {

    public BREthereumTransfer(Pointer address) {
        super(address);
    }

    public BREthereumTransfer() {
        super();
    }
}
