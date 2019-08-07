package com.breadwallet.corecrypto;

/* package */
final class Encrypter implements com.breadwallet.crypto.Encrypter {

    // TODO(fix): Implement me!

    /* package */
    static Encrypter createForAesEcb(byte[] key) {
        return null;
    }

    /* package */
    static Encrypter createForChaCha20Poly1305(com.breadwallet.crypto.Key key, byte[] nonce12, byte[] ad) {
        return null;
    }

    /* package */
    static Encrypter createForPigeon(com.breadwallet.crypto.Key privKey,
                                     com.breadwallet.crypto.Key pubKey,
                                     byte[] nonce12) {
        return null;
    }

    @Override
    public byte[] encrypt(byte[] data) {
        return new byte[0];
    }

    @Override
    public byte[] decrypt(byte[] data) {
        return new byte[0];
    }
}
