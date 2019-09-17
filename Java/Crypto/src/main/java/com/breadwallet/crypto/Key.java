/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;

import java.util.List;

public interface Key {

    static void setDefaultWordList(List<String> wordList) {
        CryptoApi.getProvider().keyProvider().setDefaultWordList(wordList);
    }

    static List<String> getDefaultWordList() {
        return CryptoApi.getProvider().keyProvider().getDefaultWordList();
    }

    static boolean isProtectedPrivateKeyString(byte[] privatekeyUtf8) {
        return CryptoApi.getProvider().keyProvider().isProtectedPrivateKeyString(privatekeyUtf8);
    }

    static Optional<Key> createFromPhrase(byte[] phraseUtf8, List<String> words) {
        return CryptoApi.getProvider().keyProvider().createFromPhrase(phraseUtf8, words);
    }

    static Optional<Key> createFromPrivateKeyString(byte[] privatekeyUtf8) {
        return CryptoApi.getProvider().keyProvider().createFromPrivateKeyString(privatekeyUtf8);
    }

    static Optional<Key> createFromPrivateKeyString(byte[] privatekeyUtf8, byte[] passphraseUtf8) {
        return CryptoApi.getProvider().keyProvider().createFromPrivateKeyString(privatekeyUtf8, passphraseUtf8);
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
