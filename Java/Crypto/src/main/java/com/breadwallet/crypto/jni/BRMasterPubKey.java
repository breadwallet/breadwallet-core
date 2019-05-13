package com.breadwallet.crypto.jni;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BRMasterPubKey extends Structure {

    public int fingerPrint;

    public UInt256 chainCode;

    public byte[] pubKey = new byte[33];

    public BRMasterPubKey() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("fingerPrint", "chainCode", "pubKey");
    }

    public BRMasterPubKey(int fingerPrint, UInt256 chainCode, byte pubKey[]) {
        super();
        this.fingerPrint = fingerPrint;
        this.chainCode = chainCode;
        if ((pubKey.length != this.pubKey.length))
            throw new IllegalArgumentException("Wrong array size !");
        this.pubKey = pubKey;
    }

    public BRMasterPubKey(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRMasterPubKey implements Structure.ByReference {

    }

    public static class ByValue extends BRMasterPubKey implements Structure.ByValue {

    }
}
