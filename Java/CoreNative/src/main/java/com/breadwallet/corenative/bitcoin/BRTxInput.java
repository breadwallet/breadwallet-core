/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.bitcoin;

import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.support.UInt256;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;

public class BRTxInput extends Structure {

    public UInt256 txHash;
    public int index;
    public byte[] address = new byte[75];
    public long amount;
    public Pointer script;
    public SizeT scriptLen;
    public Pointer signature;
    public SizeT sigLen;
    public Pointer witness;
    public SizeT witLen;
    public int sequence;

    public BRTxInput() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("txHash", "index", "address", "amount", "script", "scriptLen", "signature", "sigLen", "witness", "witLen", "sequence");
    }

    public BRTxInput(Pointer peer) {
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

    public static class ByReference extends BRTxInput implements Structure.ByReference {

    }

    public static class ByValue extends BRTxInput implements Structure.ByValue {

    }
}
