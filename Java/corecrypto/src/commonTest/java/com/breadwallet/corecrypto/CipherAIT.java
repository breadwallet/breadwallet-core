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
public class CipherAIT {

    @Test
    public void testCipher() {
        byte[] k;
        byte[] d;
        byte[] a;

        // aes-ecb

        k = new byte[] {
                (byte) 0x2b, (byte) 0x7e, (byte) 0x15, (byte) 0x16, (byte) 0x28,
                (byte) 0xae, (byte) 0xd2, (byte) 0xa6, (byte) 0xab, (byte) 0xf7,
                (byte) 0x15, (byte) 0x88, (byte) 0x09, (byte) 0xcf, (byte) 0x4f,
                (byte) 0x3c};
        d = new byte[] {
                (byte) 0x6b, (byte) 0xc1, (byte) 0xbe, (byte) 0xe2, (byte) 0x2e,
                (byte) 0x40, (byte) 0x9f, (byte) 0x96, (byte) 0xe9, (byte) 0x3d,
                (byte) 0x7e, (byte) 0x11, (byte) 0x73, (byte) 0x93, (byte) 0x17,
                (byte) 0x2a, (byte) 0xae, (byte) 0x2d, (byte) 0x8a, (byte) 0x57,
                (byte) 0x1e, (byte) 0x03, (byte) 0xac, (byte) 0x9c, (byte) 0x9e,
                (byte) 0xb7, (byte) 0x6f, (byte) 0xac, (byte) 0x45, (byte) 0xaf,
                (byte) 0x8e, (byte) 0x51, (byte) 0x30, (byte) 0xc8, (byte) 0x1c,
                (byte) 0x46, (byte) 0xa3, (byte) 0x5c, (byte) 0xe4, (byte) 0x11,
                (byte) 0xe5, (byte) 0xfb, (byte) 0xc1, (byte) 0x19, (byte) 0x1a,
                (byte) 0x0a, (byte) 0x52, (byte) 0xef, (byte) 0xf6, (byte) 0x9f,
                (byte) 0x24, (byte) 0x45, (byte) 0xdf, (byte) 0x4f, (byte) 0x9b,
                (byte) 0x17, (byte) 0xad, (byte) 0x2b, (byte) 0x41, (byte) 0x7b,
                (byte) 0xe6, (byte) 0x6c, (byte) 0x37, (byte) 0x10};
        a = Cipher.createForAesEcb(k).encrypt(d).get();
        assertArrayEquals(d, Cipher.createForAesEcb(k).decrypt(a).get());

        // cha-cha

        byte[] nonce12 = new byte[] {
                (byte) 0x07, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x40,
                (byte) 0x41, (byte) 0x42, (byte) 0x43, (byte) 0x44, (byte) 0x45,
                (byte) 0x46, (byte) 0x47};
        byte[] ad = new byte[] {
                (byte) 0x50, (byte) 0x51, (byte) 0x52, (byte) 0x53, (byte) 0xc0,
                (byte) 0xc1, (byte) 0xc2, (byte) 0xc3, (byte) 0xc4, (byte) 0xc5,
                (byte) 0xc6, (byte) 0xc7};
        byte[] msg = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.".getBytes(StandardCharsets.UTF_8);
        Key key = Key.createFromSecret(new byte[] {
                (byte) 0x80, (byte) 0x81, (byte) 0x82, (byte) 0x83, (byte) 0x84,
                (byte) 0x85, (byte) 0x86, (byte) 0x87, (byte) 0x88, (byte) 0x89,
                (byte) 0x8a, (byte) 0x8b, (byte) 0x8c, (byte) 0x8d, (byte) 0x8e,
                (byte) 0x8f, (byte) 0x90, (byte) 0x91, (byte) 0x92, (byte) 0x93,
                (byte) 0x94, (byte) 0x95, (byte) 0x96, (byte) 0x97, (byte) 0x98,
                (byte) 0x99, (byte) 0x9a, (byte) 0x9b, (byte) 0x9c, (byte) 0x9d,
                (byte) 0x9e, (byte) 0x9f
        }).get();
        Cipher alg = Cipher.createForChaCha20Poly1305(key, nonce12, ad);

        byte[] cipher = {
                (byte) 0xd3, (byte) 0x1a, (byte) 0x8d, (byte) 0x34, (byte) 0x64,
                (byte) 0x8e, (byte) 0x60, (byte) 0xdb, (byte) 0x7b, (byte) 0x86,
                (byte) 0xaf, (byte) 0xbc, (byte) 0x53, (byte) 0xef, (byte) 0x7e,
                (byte) 0xc2, (byte) 0xa4, (byte) 0xad, (byte) 0xed, (byte) 0x51,
                (byte) 0x29, (byte) 0x6e, (byte) 0x08, (byte) 0xfe, (byte) 0xa9,
                (byte) 0xe2, (byte) 0xb5, (byte) 0xa7, (byte) 0x36, (byte) 0xee,
                (byte) 0x62, (byte) 0xd6, (byte) 0x3d, (byte) 0xbe, (byte) 0xa4,
                (byte) 0x5e, (byte) 0x8c, (byte) 0xa9, (byte) 0x67, (byte) 0x12,
                (byte) 0x82, (byte) 0xfa, (byte) 0xfb, (byte) 0x69, (byte) 0xda,
                (byte) 0x92, (byte) 0x72, (byte) 0x8b, (byte) 0x1a, (byte) 0x71,
                (byte) 0xde, (byte) 0x0a, (byte) 0x9e, (byte) 0x06, (byte) 0x0b,
                (byte) 0x29, (byte) 0x05, (byte) 0xd6, (byte) 0xa5, (byte) 0xb6,
                (byte) 0x7e, (byte) 0xcd, (byte) 0x3b, (byte) 0x36, (byte) 0x92,
                (byte) 0xdd, (byte) 0xbd, (byte) 0x7f, (byte) 0x2d, (byte) 0x77,
                (byte) 0x8b, (byte) 0x8c, (byte) 0x98, (byte) 0x03, (byte) 0xae,
                (byte) 0xe3, (byte) 0x28, (byte) 0x09, (byte) 0x1b, (byte) 0x58,
                (byte) 0xfa, (byte) 0xb3, (byte) 0x24, (byte) 0xe4, (byte) 0xfa,
                (byte) 0xd6, (byte) 0x75, (byte) 0x94, (byte) 0x55, (byte) 0x85,
                (byte) 0x80, (byte) 0x8b, (byte) 0x48, (byte) 0x31, (byte) 0xd7,
                (byte) 0xbc, (byte) 0x3f, (byte) 0xf4, (byte) 0xde, (byte) 0xf0,
                (byte) 0x8e, (byte) 0x4b, (byte) 0x7a, (byte) 0x9d, (byte) 0xe5,
                (byte) 0x76, (byte) 0xd2, (byte) 0x65, (byte) 0x86, (byte) 0xce,
                (byte) 0xc6, (byte) 0x4b, (byte) 0x61, (byte) 0x16, (byte) 0x1a,
                (byte) 0xe1, (byte) 0x0b, (byte) 0x59, (byte) 0x4f, (byte) 0x09,
                (byte) 0xe2, (byte) 0x6a, (byte) 0x7e, (byte) 0x90, (byte) 0x2e,
                (byte) 0xcb, (byte) 0xd0, (byte) 0x60, (byte) 0x06, (byte) 0x91
        };
        assertArrayEquals(cipher, alg.encrypt(msg).get());
        assertArrayEquals(msg, alg.decrypt(cipher).get());

        // pigeon

        Key pubKey = Key.createFromPublicKeyString("02d404943960a71535a79679f1cf1df80e70597c05b05722839b38ebc8803af517".getBytes(StandardCharsets.UTF_8)).get();
        Cipher pigeon = Cipher.createForPigeon(key, pubKey, nonce12);
        byte [] pigeonCipher = pigeon.encrypt(msg).get();
        assertArrayEquals(msg, pigeon.decrypt(pigeonCipher).get());
    }
}
