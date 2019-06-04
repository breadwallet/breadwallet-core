package com.breadwallet.crypto.libcrypto.bitcoin;

import com.breadwallet.crypto.libcrypto.utility.SizeT;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;

public class BRTxOutput extends Structure {

    public byte[] address = new byte[75];
    public long amount;

    public Pointer script;
    public SizeT scriptLen;

    public BRTxOutput() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("address", "amount", "script", "scriptLen");
    }

    public BRTxOutput(byte address[], long amount, Pointer script, SizeT scriptLen) {
        super();
        if ((address.length != this.address.length))
            throw new IllegalArgumentException("Wrong array size !");
        this.address = address;
        this.amount = amount;
        this.script = script;
        this.scriptLen = scriptLen;
    }

    public BRTxOutput(Pointer peer) {
        super(peer);
    }

    public String getAddressAsString() {
        String addressStr = new String(address, StandardCharsets.UTF_8);

        int len = addressStr.length();
        int end = 0;

        while ((end < len) && (addressStr.charAt(end) > ' ')) {
            end++;
        }

        return addressStr.substring(0, end);
    }

    public static class ByReference extends BRTxOutput implements Structure.ByReference {

    }

    public static class ByValue extends BRTxOutput implements Structure.ByValue {

    }
}
