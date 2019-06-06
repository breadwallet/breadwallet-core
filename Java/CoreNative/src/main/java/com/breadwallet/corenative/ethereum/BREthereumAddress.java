/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.ethereum;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.crypto.BREthereumBoolean;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BREthereumAddress extends Structure {

    public static boolean isValid(String address) {
        return BREthereumBoolean.ETHEREUM_BOOLEAN_TRUE == CryptoLibrary.INSTANCE.addressValidateString(address);
    }

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
