/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.support;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.crypto.BRCryptoBoolean;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;

public class BRAddress extends Structure {

    public static boolean isValid(String address) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.BRAddressIsValid(address);
    }

    public static BRAddress addressFill(String address) {
        byte[] dstBytes = new byte[75];
        if (isValid(address)) {
            byte[] srcBytes = address.getBytes(StandardCharsets.UTF_8);
            System.arraycopy(srcBytes, 0, dstBytes, 0, Math.min(srcBytes.length, 74));
        }
        return new BRAddress(dstBytes);
    }

    public byte[] s = new byte[75];

    public BRAddress() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("s");
    }

    public BRAddress(byte s[]) {
        super();
        if ((s.length != this.s.length))
            throw new IllegalArgumentException("Wrong array size !");
        this.s = s;
    }

    public String getAddressAsString() {
        String addressStr = new String(s, StandardCharsets.UTF_8);

        int len = addressStr.length();
        int end = 0;

        while ((end < len) && (addressStr.charAt(end) > ' ')) {
            end++;
        }

        return addressStr.substring(0, end).trim();
    }

    public BRAddress(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRAddress implements Structure.ByReference {

    }

    public static class ByValue extends BRAddress implements Structure.ByValue {

        public ByValue() {
            super();
        }

        public ByValue(BRAddress address) {
            super(address.s);
        }
    }
}
