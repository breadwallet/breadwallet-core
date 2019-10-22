/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoKey;
import com.google.common.base.Optional;

import java.util.List;

/* package */
final class Key implements com.breadwallet.crypto.Key {

    @Nullable
    static private List<String> wordList;

    /* package */
    static void setDefaultWordList(List<String> wordList) {
        Key.wordList = wordList;
    }

    /* package */
    static List<String> getDefaultWordList() {
        return Key.wordList;
    }

    /* package */
    static boolean isProtectedPrivateKeyString(byte[] keyStringUtf8) {
        return BRCryptoKey.isProtectedPrivateKeyString(keyStringUtf8);
    }

    /* package */
    static Optional<Key> createFromPhrase(byte[] phraseUtf8, @Nullable List<String> words) {
        if (words == null) {
            words = Key.wordList;
        }

        if (words == null) {
            return Optional.absent();
        }

        Optional<BRCryptoKey> core = BRCryptoKey.createFromPhrase(phraseUtf8, words);
        return core.transform(Key::create);
    }

    /* package */
    static Optional<Key> createFromPrivateKeyString(byte[] keyStringUtf8) {
        Optional<BRCryptoKey> core = BRCryptoKey.createFromPrivateKeyString(keyStringUtf8);
        return core.transform(Key::create);
    }

    /* package */
    static Optional<Key> createFromPrivateKeyString(byte[] keyStringUtf8, byte[] phraseUtf8) {
        Optional<BRCryptoKey> core = BRCryptoKey.createFromPrivateKeyString(keyStringUtf8, phraseUtf8);
        return core.transform(Key::create);
    }

    /* package */
    static Optional<Key> createFromPublicKeyString(byte[] keyStringUtf8) {
        Optional<BRCryptoKey> core = BRCryptoKey.createFromPublicKeyString(keyStringUtf8);
        return core.transform(Key::create);
    }

    /* package */
    static Optional<Key> createForPigeon(com.breadwallet.crypto.Key key, byte[] nonce) {
        Optional<BRCryptoKey> core = BRCryptoKey.createForPigeon(from(key).core, nonce);
        return core.transform(Key::create);
    }

    /* package */
    static Optional<Key> createForBIP32ApiAuth(byte[] phraseUtf8, @Nullable List<String> words) {
        if (words == null) {
            words = Key.wordList;
        }

        if (words == null) {
            return Optional.absent();
        }

        Optional<BRCryptoKey> core = BRCryptoKey.createForBIP32ApiAuth(phraseUtf8, words);
        return core.transform(Key::create);
    }

    /* package */
    static Optional<Key> createForBIP32BitID(byte[] phraseUtf8, int index, String uri, @Nullable List<String> words) {
        if (words == null) {
            words = Key.wordList;
        }

        if (words == null) {
            return Optional.absent();
        }

        Optional<BRCryptoKey> core = BRCryptoKey.createForBIP32BitID(phraseUtf8, index, uri, words);
        return core.transform(Key::create);
    }

    /* package */
    static Optional<Key> createFromSecret(byte[] secret) {
        Optional<BRCryptoKey> core = BRCryptoKey.cryptoKeyCreateFromSecret(secret);
        return core.transform(Key::create);
    }

    /* package */
    static Key create(BRCryptoKey core) {
        Key key = new Key(core);
        ReferenceCleaner.register(key, core::give);
        return key;
    }

    /* package */
    static Key from(com.breadwallet.crypto.Key key) {
        if (key == null) {
            return null;
        }

        if (key instanceof Key) {
            return (Key) key;
        }

        throw new IllegalArgumentException("Unsupported key instance");
    }

    private final BRCryptoKey core;

    private Key(BRCryptoKey core) {
        this.core = core;
        this.core.providePublicKey(0, 0);
    }

    @Override
    public byte[] encodeAsPrivate() {
        return core.encodeAsPrivate();
    }

    @Override
    public byte[] encodeAsPublic() {
        return core.encodeAsPublic();
    }

    @Override
    public boolean hasSecret() {
        return core.hasSecret();
    }

    @Override
    public byte[] getSecret() {
        return core.getSecret();
    }

    @Override
    public boolean privateKeyMatch(com.breadwallet.crypto.Key other) {
        return core.privateKeyMatch(from(other).core);
    }

    @Override
    public boolean publicKeyMatch(com.breadwallet.crypto.Key other) {
        return core.publicKeyMatch(from(other).core);
    }

    /* package */
    BRCryptoKey getBRCryptoKey() {
        return core;
    }
}
