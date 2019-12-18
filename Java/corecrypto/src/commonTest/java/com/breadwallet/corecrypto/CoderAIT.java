/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import org.junit.Ignore;
import org.junit.Test;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

@Ignore
public class CoderAIT {

    @Test
    public void testCoder() {
        byte[] d;
        String a;
        String r;
        String s;

        // HEX

        d = new byte[] {(byte) 0xde, (byte)0xad, (byte)0xbe, (byte)0xef};
        a = "deadbeef";
        r = Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.HEX).encode(d).get();
        assertEquals(a, r);
        assertArrayEquals(d, Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.HEX).decode(r).get());

        // BASE58

        s = "#&$@*^(*#!^";
        assertFalse(Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.BASE58).decode(s).isPresent());

        s = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        d = Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.BASE58).decode(s).get();
        assertEquals(s, Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.BASE58).encode(d).get());

        s = "z";
        d = Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.BASE58).decode(s).get();
        assertEquals(s, Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.BASE58).encode(d).get());

        //  BASE58CHECK

        d = new byte[] {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        s = Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.BASE58CHECK).encode(d).get();
        assertArrayEquals(d, Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.BASE58CHECK).decode(s).get());

        d = new byte[] {
                (byte) 0x05, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
                (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
                (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF};
        s = Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.BASE58CHECK).encode(d).get();
        assertArrayEquals(d, Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.BASE58CHECK).decode(s).get());
    }
}
