package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.BRCryptoKey;
import com.google.common.base.Optional;

import java.util.List;

/* package */
final class Key implements com.breadwallet.crypto.Key {

    /* package */
    static Optional<Key> createFromPhrase(byte[] phraseUtf8, List<String> words) {
        return BRCryptoKey.createFromPhrase(phraseUtf8, words).transform(Key::new);
    }

    /* package */
    static Optional<Key> createFromPrivateKeyString(byte[] privateData) {
        return BRCryptoKey.createFromPrivateKeyString(privateData).transform(Key::new);
    }

    /* package */
    static Optional<Key> createFromPublicKeyString(byte[] publicData) {
        return BRCryptoKey.createFromPublicKeyString(publicData).transform(Key::new);
    }

    /* package */
    static Optional<Key> createForPigeon(com.breadwallet.crypto.Key key, byte[] nonce) {
        return BRCryptoKey.createForPigeon(from(key).core, nonce).transform(Key::new);
    }

    /* package */
    static Optional<Key> createForBIP32ApiAuth(byte[] phraseUtf8, List<String> words) {
        return BRCryptoKey.createForBIP32ApiAuth(phraseUtf8, words).transform(Key::new);
    }

    /* package */
    static Optional<Key> createForBIP32BitID(byte[] phraseUtf8, int index, String uri, List<String> words) {
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
    }

    @Override
    public boolean hasSecret() {
        return core.hasSecret();
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
    public byte[] getSecret() {
        return core.getSecret();
    }

    @Override
    public boolean privateKeyMatch(com.breadwallet.crypto.Key other) {
        Key cryptoKey = from(other);
        return core.privateKeyMatch(cryptoKey.core);
    }

    @Override
    public boolean publicKeyMatch(com.breadwallet.crypto.Key other) {
        Key cryptoKey = from(other);
        return core.publicKeyMatch(cryptoKey.core);
    }
}
