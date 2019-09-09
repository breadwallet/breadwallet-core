package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.BRCryptoKey;
import com.google.common.base.Optional;

import java.util.List;

import javax.annotation.Nullable;

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

        return BRCryptoKey.createFromPhrase(phraseUtf8, words).transform(Key::new);
    }

    /* package */
    static Optional<Key> createFromPrivateKeyString(byte[] keyStringUtf8) {
        return BRCryptoKey.createFromPrivateKeyString(keyStringUtf8).transform(Key::new);
    }

    /* package */
    static Optional<Key> createFromPrivateKeyString(byte[] keyStringUtf8, byte[] phraseUtf8) {
        return BRCryptoKey.createFromPrivateKeyString(keyStringUtf8, phraseUtf8).transform(Key::new);
    }

    /* package */
    static Optional<Key> createFromPublicKeyString(byte[] keyStringUtf8) {
        return BRCryptoKey.createFromPublicKeyString(keyStringUtf8).transform(Key::new);
    }

    /* package */
    static Optional<Key> createForPigeon(com.breadwallet.crypto.Key key, byte[] nonce) {
        return BRCryptoKey.createForPigeon(from(key).core, nonce).transform(Key::new);
    }

    /* package */
    static Optional<Key> createForBIP32ApiAuth(byte[] phraseUtf8, @Nullable List<String> words) {
        if (words == null) {
            words = Key.wordList;
        }

        if (words == null) {
            return Optional.absent();
        }

        return BRCryptoKey.createForBIP32ApiAuth(phraseUtf8, words).transform(Key::new);
    }

    /* package */
    static Optional<Key> createForBIP32BitID(byte[] phraseUtf8, int index, String uri, @Nullable List<String> words) {
        if (words == null) {
            words = Key.wordList;
        }

        if (words == null) {
            return Optional.absent();
        }

        return BRCryptoKey.createForBIP32BitID(phraseUtf8, index, uri, words).transform(Key::new);
    }

    /* package */
    static Key create(BRCryptoKey core) {
        return new Key(core);
    }

    /* package */
    static Key from(com.breadwallet.crypto.Key key) {
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
