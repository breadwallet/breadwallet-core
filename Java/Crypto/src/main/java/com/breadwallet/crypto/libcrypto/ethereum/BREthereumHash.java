package com.breadwallet.crypto.libcrypto.ethereum;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BREthereumHash extends Structure {

    public byte[] u8 = new byte[256 / 8];

    public BREthereumHash() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("u8");
    }

    public BREthereumHash(byte u8[]) {
        super();
        if ((u8.length != this.u8.length)) {
            throw new IllegalArgumentException("Wrong array size!");
        }
        this.u8 = u8;
    }

    public BREthereumHash(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BREthereumHash implements Structure.ByReference {

    }

    public static class ByValue extends BREthereumHash implements Structure.ByValue {

    }
}
