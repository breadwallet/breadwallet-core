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

import java.nio.charset.StandardCharsets;

import static org.junit.Assert.assertArrayEquals;

@Ignore
public class HasherAIT {

    @Test
    public void testHasher() {
        byte[] d;
        byte[] a;

        // SHA1

        d = "Free online SHA1 Calculator, type text here...".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0x6f, (byte) 0xc2, (byte) 0xe2, (byte) 0x51, (byte) 0x72,
                (byte) 0xcb, (byte) 0x15, (byte) 0x19, (byte) 0x3c, (byte) 0xb1,
                (byte) 0xc6, (byte) 0xd4, (byte) 0x8f, (byte) 0x60, (byte) 0x7d,
                (byte) 0x42, (byte) 0xc1, (byte) 0xd2, (byte) 0xa2, (byte) 0x15
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA1).hash(d).get());

        // SHA256

        d = "Free online SHA256 Calculator, type text here...".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0x43, (byte) 0xfd, (byte) 0x9d, (byte) 0xeb, (byte) 0x93,
                (byte) 0xf6, (byte) 0xe1, (byte) 0x4d, (byte) 0x41, (byte) 0x82,
                (byte) 0x66, (byte) 0x04, (byte) 0x51, (byte) 0x4e, (byte) 0x3d,
                (byte) 0x78, (byte) 0x73, (byte) 0xa5, (byte) 0x49, (byte) 0xac,
                (byte) 0x87, (byte) 0xae, (byte) 0xbe, (byte) 0xbf, (byte) 0x3d,
                (byte) 0x1c, (byte) 0x10, (byte) 0xad, (byte) 0x6e, (byte) 0xb0,
                (byte) 0x57, (byte) 0xd0
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA256).hash(d).get());

        // SHA256_2

        d = "Free online SHA256_2 Calculator, type text here...".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0xe3, (byte) 0x9a, (byte) 0xee, (byte) 0xfe, (byte) 0xd2,
                (byte) 0x39, (byte) 0x02, (byte) 0xd7, (byte) 0x81, (byte) 0xef,
                (byte) 0xdc, (byte) 0x83, (byte) 0x8a, (byte) 0x0b, (byte) 0x5d,
                (byte) 0x16, (byte) 0xc0, (byte) 0xab, (byte) 0x90, (byte) 0x1d,
                (byte) 0xc9, (byte) 0x26, (byte) 0xbb, (byte) 0x6e, (byte) 0x2c,
                (byte) 0x1e, (byte) 0xdf, (byte) 0xb9, (byte) 0xc1, (byte) 0x8d,
                (byte) 0x89, (byte) 0xb8
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA256_2).hash(d).get());

        // SHA224

        d = "Free online SHA224 Calculator, type text here...".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0x09, (byte) 0xcd, (byte) 0xa9, (byte) 0x39, (byte) 0xab,
                (byte) 0x1d, (byte) 0x6e, (byte) 0x7c, (byte) 0x3f, (byte) 0x81,
                (byte) 0x3b, (byte) 0xa2, (byte) 0x3a, (byte) 0xf3, (byte) 0x4b,
                (byte) 0xdf, (byte) 0xe9, (byte) 0x35, (byte) 0x50, (byte) 0x6d,
                (byte) 0xc4, (byte) 0x92, (byte) 0xeb, (byte) 0x77, (byte) 0xd8,
                (byte) 0x22, (byte) 0x6a, (byte) 0x64
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA224).hash(d).get());

        // SHA384

        d = "Free online SHA384 Calculator, type text here...".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0xef, (byte) 0x82, (byte) 0x38, (byte) 0x77, (byte) 0xa4,
                (byte) 0x66, (byte) 0x4c, (byte) 0x96, (byte) 0x41, (byte) 0xc5,
                (byte) 0x3a, (byte) 0xc2, (byte) 0x05, (byte) 0x59, (byte) 0xc3,
                (byte) 0x4b, (byte) 0x5d, (byte) 0x2c, (byte) 0x67, (byte) 0x94,
                (byte) 0x77, (byte) 0xde, (byte) 0x22, (byte) 0xff, (byte) 0xfa,
                (byte) 0xb3, (byte) 0x51, (byte) 0xe5, (byte) 0xe3, (byte) 0x3e,
                (byte) 0xa5, (byte) 0x3e, (byte) 0x42, (byte) 0x36, (byte) 0x15,
                (byte) 0xe1, (byte) 0xee, (byte) 0x3c, (byte) 0x85, (byte) 0xe0,
                (byte) 0xd7, (byte) 0xfa, (byte) 0xcb, (byte) 0x84, (byte) 0xdf,
                (byte) 0x2b, (byte) 0xa2, (byte) 0x17
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA384).hash(d).get());

        // SHA512

        d = "Free online SHA512 Calculator, type text here...".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0x04, (byte) 0xf1, (byte) 0x15, (byte) 0x41, (byte) 0x35,
                (byte) 0xee, (byte) 0xcb, (byte) 0xe4, (byte) 0x2e, (byte) 0x9a,
                (byte) 0xdc, (byte) 0x8e, (byte) 0x1d, (byte) 0x53, (byte) 0x2f,
                (byte) 0x9c, (byte) 0x60, (byte) 0x7a, (byte) 0x84, (byte) 0x47,
                (byte) 0xb7, (byte) 0x86, (byte) 0x37, (byte) 0x7d, (byte) 0xb8,
                (byte) 0x44, (byte) 0x7d, (byte) 0x11, (byte) 0xa5, (byte) 0xb2,
                (byte) 0x23, (byte) 0x2c, (byte) 0xdd, (byte) 0x41, (byte) 0x9b,
                (byte) 0x86, (byte) 0x39, (byte) 0x22, (byte) 0x4f, (byte) 0x78,
                (byte) 0x7a, (byte) 0x51, (byte) 0xd1, (byte) 0x10, (byte) 0xf7,
                (byte) 0x25, (byte) 0x91, (byte) 0xf9, (byte) 0x64, (byte) 0x51,
                (byte) 0xa1, (byte) 0xbb, (byte) 0x51, (byte) 0x1c, (byte) 0x4a,
                (byte) 0x82, (byte) 0x9e, (byte) 0xd0, (byte) 0xa2, (byte) 0xec,
                (byte) 0x89, (byte) 0x13, (byte) 0x21, (byte) 0xf3
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA512).hash(d).get());

        // SHA3

        d = "abc".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0x3a, (byte) 0x98, (byte) 0x5d, (byte) 0xa7, (byte) 0x4f,
                (byte) 0xe2, (byte) 0x25, (byte) 0xb2, (byte) 0x04, (byte) 0x5c,
                (byte) 0x17, (byte) 0x2d, (byte) 0x6b, (byte) 0xd3, (byte) 0x90,
                (byte) 0xbd, (byte) 0x85, (byte) 0x5f, (byte) 0x08, (byte) 0x6e,
                (byte) 0x3e, (byte) 0x9d, (byte) 0x52, (byte) 0x5b, (byte) 0x46,
                (byte) 0xbf, (byte) 0xe2, (byte) 0x45, (byte) 0x11, (byte) 0x43,
                (byte) 0x15, (byte) 0x32
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA3).hash(d).get());

        // RMD160

        d = "Free online RIPEMD160 Calculator, type text here...".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0x95, (byte) 0x01, (byte) 0xa5, (byte) 0x6f, (byte) 0xb8,
                (byte) 0x29, (byte) 0x13, (byte) 0x2b, (byte) 0x87, (byte) 0x48,
                (byte) 0xf0, (byte) 0xcc, (byte) 0xc4, (byte) 0x91, (byte) 0xf0,
                (byte) 0xec, (byte) 0xbc, (byte) 0x7f, (byte) 0x94, (byte) 0x5b
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.RMD160).hash(d).get());

        // HASH160

        d = "Free online HASH160 Calculator, type text here...".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0x62, (byte) 0x0a, (byte) 0x75, (byte) 0x2d, (byte) 0x20,
                (byte) 0x09, (byte) 0xd4, (byte) 0xc6, (byte) 0x59, (byte) 0x8b,
                (byte) 0x7f, (byte) 0x63, (byte) 0x4d, (byte) 0x34, (byte) 0xc5,
                (byte) 0xec, (byte) 0xd5, (byte) 0x23, (byte) 0x36, (byte) 0x72
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.HASH160).hash(d).get());

        // KECCAK256

        d = "".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0xc5, (byte) 0xd2, (byte) 0x46, (byte) 0x01, (byte) 0x86,
                (byte) 0xf7, (byte) 0x23, (byte) 0x3c, (byte) 0x92, (byte) 0x7e,
                (byte) 0x7d, (byte) 0xb2, (byte) 0xdc, (byte) 0xc7, (byte) 0x03,
                (byte) 0xc0, (byte) 0xe5, (byte) 0x00, (byte) 0xb6, (byte) 0x53,
                (byte) 0xca, (byte) 0x82, (byte) 0x27, (byte) 0x3b, (byte) 0x7b,
                (byte) 0xfa, (byte) 0xd8, (byte) 0x04, (byte) 0x5d, (byte) 0x85,
                (byte) 0xa4, (byte) 0x70
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.KECCAK256).hash(d).get());

        // MD5

        d = "Free online MD5 Calculator, type text here...".getBytes(StandardCharsets.UTF_8);
        a = new byte[] {
                (byte) 0x0b, (byte) 0x3b, (byte) 0x20, (byte) 0xea, (byte) 0xf1,
                (byte) 0x69, (byte) 0x64, (byte) 0x62, (byte) 0xf5, (byte) 0x0d,
                (byte) 0x1a, (byte) 0x3b, (byte) 0xbd, (byte) 0xd3, (byte) 0x0c,
                (byte) 0xef
        };
        assertArrayEquals(a, Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.MD5).hash(d).get());
    }
}
