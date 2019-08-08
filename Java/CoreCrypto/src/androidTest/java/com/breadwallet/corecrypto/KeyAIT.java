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
    }
}
