package com.breadwallet.corecrypto;

import com.google.common.base.Optional;

import org.junit.Test;

import java.nio.charset.StandardCharsets;

import static org.junit.Assert.*;

public class KeyAIT {

    @Test
    public void testKey() {
        byte[] s;
        byte[] t;
        Optional<Key> k;
        Optional<Key> l;
        boolean p;

        //
        // Password Protected
        //

        // mainnet (https://bitcoinpaperwallet.com/bitcoinpaperwallet/generate-wallet.html?design=alt-testnet)
        s = "6PRPGFR3vou5h9VXHVTUpnDisZpnKik5c7zWJrw8abi4AYW8fy4uPFYFXk".getBytes(StandardCharsets.UTF_8);
        p = Key.isProtectedPrivateKeyString(s);
        assertTrue(p);
        k = Key.createFromPrivateKeyString(s, "hodl".getBytes(StandardCharsets.UTF_8));
        assertTrue(k.isPresent());
        assertArrayEquals ("5KYkuC2SX6twF8C4yhDRJsy9tgBnn9aFsEXgaMLwRciopuRnBfT".getBytes(StandardCharsets.UTF_8), k.get().encodeAsPrivate());

        // testnet (https://bitcoinpaperwallet.com/bitcoinpaperwallet/generate-wallet.html?design=alt-testnet)
        s = "6PRVDGjn5m1Tj6wbP8kG6YozjE5qBHxE9BPfF8HxZHLdd1tAkENLjjsQve".getBytes(StandardCharsets.UTF_8);
        p = Key.isProtectedPrivateKeyString(s);
        assertTrue(p);
        k = Key.createFromPrivateKeyString(s, "hodl".getBytes(StandardCharsets.UTF_8));
        assertTrue(k.isPresent());
        assertArrayEquals ("92HBCaQbnqkGkSm8z2rxKA3DRdSrsniEswiWFe5CQXdfPqThR9N".getBytes(StandardCharsets.UTF_8), k.get().encodeAsPrivate());

        s = "5Kb8kLf9zgWQnogidDA76MzPL6TsZZY36hWXMssSzNydYXYB9KF".getBytes(StandardCharsets.UTF_8);
        p = Key.isProtectedPrivateKeyString(s);
        assertFalse(p);
        k = Key.createFromPrivateKeyString(s, "hodl".getBytes(StandardCharsets.UTF_8));
        assertFalse(k.isPresent());

        s = "KyvGbxRUoofdw3TNydWn2Z78dBHSy2odn1d3wXWN2o3SAtccFNJL".getBytes(StandardCharsets.UTF_8);
        p = Key.isProtectedPrivateKeyString(s);
        assertFalse(p);
        k = Key.createFromPrivateKeyString(s, "hodl".getBytes(StandardCharsets.UTF_8));
        assertFalse(k.isPresent());

        //
        // Uncompressed
        //

        s = "5Kb8kLf9zgWQnogidDA76MzPL6TsZZY36hWXMssSzNydYXYB9KF".getBytes(StandardCharsets.UTF_8);
        k = Key.createFromPrivateKeyString(s);
        assertTrue(k.isPresent());
        assertTrue(k.get().hasSecret());
        assertArrayEquals(s, k.get().encodeAsPrivate());

        //
        // Compressed
        //

        s = "KyvGbxRUoofdw3TNydWn2Z78dBHSy2odn1d3wXWN2o3SAtccFNJL".getBytes(StandardCharsets.UTF_8);
        k = Key.createFromPrivateKeyString(s);
        assertTrue(k.isPresent());
        assertTrue(k.get().hasSecret());
        assertArrayEquals(s, k.get().encodeAsPrivate());
        l = Key.createFromPrivateKeyString(k.get().encodeAsPrivate());
        assertTrue(k.get().privateKeyMatch(l.get()));

        t = k.get().encodeAsPublic();
        l = Key.createFromPublicKeyString(t);
        assertTrue(l.isPresent());
        assertFalse(l.get().hasSecret());
        assertArrayEquals(t, l.get().encodeAsPublic());

        assertTrue (l.get().publicKeyMatch(k.get()));

        //
        // Bad Key
        //

        s = "XyvGbxRUoofdw3TNydWn2Z78dBHSy2odn1d3wXWN2o3SAtccFNJL".getBytes(StandardCharsets.UTF_8);
        k = Key.createFromPrivateKeyString(s);
        assertFalse(k.isPresent());

        //
        // Phrase
        //
        k = Key.createFromPhrase("nothing".getBytes(StandardCharsets.UTF_8), HelpersAIT.BIP39_WORDS_EN);
        assertFalse(k.isPresent());
        k = Key.createFromPhrase("ginger settle marine tissue robot crane night number ramp coast roast critic".getBytes(StandardCharsets.UTF_8), HelpersAIT.BIP39_WORDS_EN);
        assertTrue(k.isPresent());;

        // Pigeon

        // BIP32ApiAuth

        // BIP32BitID
    }
}
