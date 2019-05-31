package com.breadwallet.crypto.libcrypto.bitcoin;

import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRChainParams extends PointerType {
    public BRChainParams(Pointer address) {
        super(address);
    }
    public BRChainParams() {
        super();
    }
}
