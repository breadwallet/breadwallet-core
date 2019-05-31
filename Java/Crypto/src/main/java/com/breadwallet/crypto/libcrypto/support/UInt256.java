package com.breadwallet.crypto.libcrypto.support;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class UInt256 extends Structure {

    public byte[] u8 = new byte[256 / 8];

    public UInt256() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("u8");
    }

    public UInt256(byte u8[]) {
        super();
        if ((u8.length != this.u8.length)) {
            throw new IllegalArgumentException("Wrong array size!");
        }
        this.u8 = u8;
    }

    public UInt256(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends UInt256 implements Structure.ByReference {

    }

    public static class ByValue extends UInt256 implements Structure.ByValue {

    }
}
