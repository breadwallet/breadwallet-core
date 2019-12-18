/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.google.common.base.Optional;

import org.junit.Ignore;
import org.junit.Test;

import java.nio.charset.StandardCharsets;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

@Ignore
public class SignerAIT {

    @Test
    public void testSigner() {
        String msg;
        byte[] digest;
        byte[] signature;
        Signer signer;
        byte[] answer;

        Key key = Key.createFromSecret(new byte[] {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}).get();

        // Basic DER

        msg = "How wonderful that we have met with a paradox. Now we have some hope of making progress.";
        digest = Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA256).hash(msg.getBytes(StandardCharsets.UTF_8)).get();
        signer = Signer.createForAlgorithm(com.breadwallet.crypto.Signer.Algorithm.BASIC_DER);
        signature = signer.sign(digest, key).get();

        answer = new byte[] {
                (byte) 0x30, (byte) 0x45, (byte) 0x02, (byte) 0x21, (byte) 0x00,
                (byte) 0xc0, (byte) 0xda, (byte) 0xfe, (byte) 0xc8, (byte) 0x25,
                (byte) 0x1f, (byte) 0x1d, (byte) 0x50, (byte) 0x10, (byte) 0x28,
                (byte) 0x9d, (byte) 0x21, (byte) 0x02, (byte) 0x32, (byte) 0x22,
                (byte) 0x0b, (byte) 0x03, (byte) 0x20, (byte) 0x2c, (byte) 0xba,
                (byte) 0x34, (byte) 0xec, (byte) 0x11, (byte) 0xfe, (byte) 0xc5,
                (byte) 0x8b, (byte) 0x3e, (byte) 0x93, (byte) 0xa8, (byte) 0x5b,
                (byte) 0x91, (byte) 0xd3, (byte) 0x02, (byte) 0x20, (byte) 0x75,
                (byte) 0xaf, (byte) 0xdc, (byte) 0x06, (byte) 0xb7, (byte) 0xd6,
                (byte) 0x32, (byte) 0x2a, (byte) 0x59, (byte) 0x09, (byte) 0x55,
                (byte) 0xbf, (byte) 0x26, (byte) 0x4e, (byte) 0x7a, (byte) 0xaa,
                (byte) 0x15, (byte) 0x58, (byte) 0x47, (byte) 0xf6, (byte) 0x14,
                (byte) 0xd8, (byte) 0x00, (byte) 0x78, (byte) 0xa9, (byte) 0x02,
                (byte) 0x92, (byte) 0xfe, (byte) 0x20, (byte) 0x50, (byte) 0x64,
                (byte) 0xd3};
        assertArrayEquals(answer, signature);

        // Basic JOSE

        msg = "How wonderful that we have met with a paradox. Now we have some hope of making progress.";
        digest = Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA256).hash(msg.getBytes(StandardCharsets.UTF_8)).get();
        signer = Signer.createForAlgorithm(com.breadwallet.crypto.Signer.Algorithm.BASIC_JOSE);
        signature = signer.sign(digest, key).get();

        answer = new byte[] {
                (byte) 0xc0, (byte) 0xda, (byte) 0xfe, (byte) 0xc8, (byte) 0x25,
                (byte) 0x1f, (byte) 0x1d, (byte) 0x50, (byte) 0x10, (byte) 0x28,
                (byte) 0x9d, (byte) 0x21, (byte) 0x02, (byte) 0x32, (byte) 0x22,
                (byte) 0x0b, (byte) 0x03, (byte) 0x20, (byte) 0x2c, (byte) 0xba,
                (byte) 0x34, (byte) 0xec, (byte) 0x11, (byte) 0xfe, (byte) 0xc5,
                (byte) 0x8b, (byte) 0x3e, (byte) 0x93, (byte) 0xa8, (byte) 0x5b,
                (byte) 0x91, (byte) 0xd3, (byte) 0x75, (byte) 0xaf, (byte) 0xdc,
                (byte) 0x06, (byte) 0xb7, (byte) 0xd6, (byte) 0x32, (byte) 0x2a,
                (byte) 0x59, (byte) 0x09, (byte) 0x55, (byte) 0xbf, (byte) 0x26,
                (byte) 0x4e, (byte) 0x7a, (byte) 0xaa, (byte) 0x15, (byte) 0x58,
                (byte) 0x47, (byte) 0xf6, (byte) 0x14, (byte) 0xd8, (byte) 0x00,
                (byte) 0x78, (byte) 0xa9, (byte) 0x02, (byte) 0x92, (byte) 0xfe,
                (byte) 0x20, (byte) 0x50, (byte) 0x64, (byte) 0xd3};
        assertArrayEquals(answer, signature);

        // Compact

        msg = "foo";
        digest = Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA256).hash(msg.getBytes(StandardCharsets.UTF_8)).get();
        signer = Signer.createForAlgorithm(com.breadwallet.crypto.Signer.Algorithm.COMPACT);
        signature = signer.sign(digest, key).get();

        Optional<Key> keyPublic = signer.recover(digest, signature);
        assertTrue(keyPublic.isPresent());
        assertTrue(key.publicKeyMatch(keyPublic.get()));
    }

    @Test
    public void testCompactSigner() {
        byte[][] secrets = new byte[][] {
                "5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj".getBytes(StandardCharsets.UTF_8),
                "5KC4ejrDjv152FGwP386VD1i2NYc5KkfSMyv1nGy1VGDxGHqVY3".getBytes(StandardCharsets.UTF_8),
                "Kwr371tjA9u2rFSMZjTNun2PXXP3WPZu2afRHTcta6KxEUdm1vEw".getBytes(StandardCharsets.UTF_8),
                "L3Hq7a8FEQwJkW1M2GNKDW28546Vp5miewcCzSqUD9kCAXrJdS3g".getBytes(StandardCharsets.UTF_8),
        };

        String[] signatures = new String[] {
                "1c5dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d14ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6",
                "1c52d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd561d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d",
                "205dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d14ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6",
                "2052d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd561d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d",
        };

        byte[] message = Hasher.createForAlgorithm(com.breadwallet.crypto.Hasher.Algorithm.SHA256_2)
                .hash("Very deterministic message".getBytes(StandardCharsets.UTF_8)).get();


        for (int i = 0; i < secrets.length; i++) {
            Optional<Key> maybeKey = Key.createFromPrivateKeyString(secrets[i]);
            assertTrue(maybeKey.isPresent());

            byte[] outputSig = Signer.createForAlgorithm(com.breadwallet.crypto.Signer.Algorithm.COMPACT).sign(message, maybeKey.get()).get();
            String outputSigHex = Coder.createForAlgorithm(com.breadwallet.crypto.Coder.Algorithm.HEX).encode(outputSig).get();
            assertEquals(outputSigHex, signatures[i]);
        }
    }
}
