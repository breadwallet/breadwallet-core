/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.BRCryptoCipher;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
final class Encrypter implements com.breadwallet.crypto.Encrypter {

    /* package */
    static Encrypter createForAesEcb(byte[] key) {
        BRCryptoCipher cipher = BRCryptoCipher.createAesEcb(key).orNull();
        checkNotNull(cipher);
        return new Encrypter(cipher);
    }

    /* package */
    static Encrypter createForChaCha20Poly1305(com.breadwallet.crypto.Key key, byte[] nonce12, byte[] ad) {
        BRCryptoCipher cipher = BRCryptoCipher.createChaCha20Poly1305(
                Key.from(key).getBRCryptoKey(),
                nonce12,
                ad)
                .orNull();
        checkNotNull(cipher);
        return new Encrypter(cipher);
    }

    /* package */
    static Encrypter createForPigeon(com.breadwallet.crypto.Key privKey,
                                     com.breadwallet.crypto.Key pubKey,
                                     byte[] nonce12) {
        BRCryptoCipher cipher = BRCryptoCipher.createPigeon(
                Key.from(privKey).getBRCryptoKey(),
                Key.from(pubKey).getBRCryptoKey(),
                nonce12)
                .orNull();
        checkNotNull(cipher);
        return new Encrypter(cipher);
    }

    private final BRCryptoCipher core;

    private Encrypter(BRCryptoCipher core) {
        this.core = core;
    }

    @Override
    public byte[] encrypt(byte[] data) {
        return core.encrypt(data);
    }

    @Override
    public byte[] decrypt(byte[] data) {
        return core.decrypt(data);
    }
}
