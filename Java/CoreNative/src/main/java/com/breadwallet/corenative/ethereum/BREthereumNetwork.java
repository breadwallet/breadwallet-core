package com.breadwallet.corenative.ethereum;

import com.breadwallet.corenative.CryptoLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BREthereumNetwork extends PointerType {

    public BREthereumNetwork(Pointer address) {
        super(address);
    }

    public BREthereumNetwork() {
        super();
    }

    public String getName() {
        return CryptoLibrary.INSTANCE.networkGetName(this).getString(0, "UTF-8");
    }
}
