/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.support;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class UInt512 extends Structure {

    public byte[] u8 = new byte[512 / 8];

    public UInt512() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("u8");
    }

    public UInt512(byte u8[]) {
        super();
        if ((u8.length != this.u8.length)) {
            throw new IllegalArgumentException("Wrong array size!");
        }
        this.u8 = u8;
    }

    public UInt512(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends UInt512 implements com.sun.jna.Structure.ByReference {

    }

    public static class ByValue extends UInt512 implements com.sun.jna.Structure.ByValue {

    }
}
