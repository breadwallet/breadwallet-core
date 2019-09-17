/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

public interface Encrypter {

    static Encrypter createForAesEcb(byte[] key) {
        return CryptoApi.getProvider().encrypterProvider().createEncrypterrForAesEcb(key);
    }

    static Encrypter createForChaCha20Poly1305(Key key, byte[] nonce12, byte[] ad) {
        return CryptoApi.getProvider().encrypterProvider().createEncrypterForChaCha20Poly1305(key, nonce12, ad);
    }

    static Encrypter createForPigeon(Key privKey, Key pubKey, byte[] nonce12) {
        return CryptoApi.getProvider().encrypterProvider().createEncrypterForPigeon(privKey, pubKey, nonce12);
    }

    byte[] encrypt(byte[] data);

    byte[] decrypt(byte[] data);
}
