/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;

public interface Cipher {

    static Cipher createForAesEcb(byte[] key) {
        return CryptoApi.getProvider().cipherProvider().createCipherForAesEcb(key);
    }

    static Cipher createForChaCha20Poly1305(Key key, byte[] nonce12, byte[] ad) {
        return CryptoApi.getProvider().cipherProvider().createCipherForChaCha20Poly1305(key, nonce12, ad);
    }

    static Cipher createForPigeon(Key privKey, Key pubKey, byte[] nonce12) {
        return CryptoApi.getProvider().cipherProvider().createCipherForPigeon(privKey, pubKey, nonce12);
    }

    Optional<byte[]> encrypt(byte[] data);

    Optional<byte[]> decrypt(byte[] data);
}
