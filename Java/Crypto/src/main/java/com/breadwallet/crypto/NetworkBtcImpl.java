package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.bitcoin.BRChainParams;
import com.sun.jna.Pointer;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
class NetworkBtcImpl {

    /* package */
    final BRChainParams core;

    /* package */ NetworkBtcImpl(boolean isMainnet) {
        Pointer ptr = CryptoLibrary.LIBRARY.getGlobalVariableAddress(isMainnet ? "BRMainNetParams" : "BRTestNetParams");
        checkNotNull(ptr);
        this.core = new BRChainParams(ptr.getPointer(0));
    }
}
