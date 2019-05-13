package com.breadwallet.crypto.jni;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BREthereumAddress extends Structure {

    public byte[] s = new byte[20];

    public BREthereumAddress() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("s");
    }

    public BREthereumAddress(byte s[]) {
        super();
        if ((s.length != this.s.length))
            throw new IllegalArgumentException("Wrong array size !");
        this.s = s;
    }

    public BREthereumAddress(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BREthereumAddress implements Structure.ByReference {

    }

    public static class ByValue extends BREthereumAddress implements Structure.ByValue {

        public ByValue() {
            super();
        }

        public ByValue(BREthereumAddress address) {
            super(address.s);
        }
    }
}
