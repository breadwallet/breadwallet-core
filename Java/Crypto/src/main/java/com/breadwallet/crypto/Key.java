package com.breadwallet.crypto;

import com.google.common.base.Optional;

import java.util.List;

public interface Key {

    static Optional<Key> createFromPhrase(byte[] phraseUtf8, List<String> words) {
        return CryptoApi.getProvider().keyProvider().createFromPhrase(phraseUtf8, words);
    }

    static Optional<Key> createFromPrivateKeyString(byte[] privatekeyUtf8) {
        return CryptoApi.getProvider().keyProvider().createFromPrivateKeyString(privatekeyUtf8);
    }

    static Optional<Key> createFromPublicKeyString(byte[] publicKeyUtf8) {
        return CryptoApi.getProvider().keyProvider().createFromPublicKeyString(publicKeyUtf8);
    }

    static Optional<Key> createForPigeon(Key key, byte[] nonce) {
        return CryptoApi.getProvider().keyProvider().createForPigeon(key, nonce);
    }

    static Optional<Key> createForBIP32ApiAuth(byte[] phraseUtf8, List<String> words) {
        return CryptoApi.getProvider().keyProvider().createForBIP32ApiAuth(phraseUtf8, words);
    }

    static Optional<Key> createForBIP32BitID(byte[] phraseUtf8, int index, String uri, List<String> words) {
        return CryptoApi.getProvider().keyProvider().createForBIP32BitID(phraseUtf8, index, uri, words);
    }

    boolean hasSecret();

    byte[] encodeAsPrivate();

    byte[] encodeAsPublic();

    byte[] getSecret();

    boolean privateKeyMatch(Key other);

    boolean publicKeyMatch(Key other);
}
