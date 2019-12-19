/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoCipher;
import com.google.common.base.Optional;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
final class Cipher implements com.breadwallet.crypto.Cipher {

    /* package */
    static Optional<byte[]> migrateBRCoreKeyCiphertext(com.breadwallet.crypto.Key key, byte[] nonce12, byte[] authenticatedData,
                                                       byte[] ciphertext) {
        return Cipher.createForChaCha20Poly1305(key, nonce12, authenticatedData).migrateBRCoreKeyCiphertext(ciphertext);
    }

    /* package */
    static Cipher createForAesEcb(byte[] key) {
        BRCryptoCipher cipher = BRCryptoCipher.createAesEcb(key).orNull();
        checkNotNull(cipher);
        return Cipher.create(cipher);
    }

    /* package */
    static Cipher createForChaCha20Poly1305(com.breadwallet.crypto.Key key, byte[] nonce12, byte[] authenticatedData) {
        BRCryptoCipher cipher = BRCryptoCipher.createChaCha20Poly1305(
                Key.from(key).getBRCryptoKey(),
                nonce12,
                authenticatedData)
                .orNull();
        checkNotNull(cipher);
        return Cipher.create(cipher);
    }

    /* package */
    static Cipher createForPigeon(com.breadwallet.crypto.Key privKey,
                                  com.breadwallet.crypto.Key pubKey,
                                  byte[] nonce12) {
        BRCryptoCipher cipher = BRCryptoCipher.createPigeon(
                Key.from(privKey).getBRCryptoKey(),
                Key.from(pubKey).getBRCryptoKey(),
                nonce12)
                .orNull();
        checkNotNull(cipher);
        return Cipher.create(cipher);
    }

    private static Cipher create(BRCryptoCipher core) {
        Cipher cipher = new Cipher(core);
        ReferenceCleaner.register(cipher, core::give);
        return cipher;
    }

    private final BRCryptoCipher core;

    private Cipher(BRCryptoCipher core) {
        this.core = core;
    }

    @Override
    public Optional<byte[]> encrypt(byte[] data) {
        return core.encrypt(data);
    }

    @Override
    public Optional<byte[]> decrypt(byte[] data) {
        return core.decrypt(data);
    }

    private Optional<byte[]> migrateBRCoreKeyCiphertext(byte[] data) {
        return core.migrateBRCoreKeyCiphertext(data);
    }
}
