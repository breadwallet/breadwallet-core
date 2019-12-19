/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.google.common.primitives.Ints;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoCipher extends PointerType {

    public static Optional<BRCryptoCipher> createAesEcb(byte[] key) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoCipherCreateForAESECB(key, new SizeT(key.length))
        ).transform(BRCryptoCipher::new);
    }

    public static Optional<BRCryptoCipher> createChaCha20Poly1305(BRCryptoKey key, byte[] nonce12, byte[] ad) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoCipherCreateForChacha20Poly1305(
                        key.getPointer(),
                        nonce12,
                        new SizeT(nonce12.length),
                        ad,
                        new SizeT(ad.length))
        ).transform(BRCryptoCipher::new);
    }

    public static Optional<BRCryptoCipher> createPigeon(BRCryptoKey privKey, BRCryptoKey pubKey, byte[] nonce12) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoCipherCreateForPigeon(
                        privKey.getPointer(),
                        pubKey.getPointer(),
                        nonce12,
                        new SizeT(nonce12.length))
        ).transform(BRCryptoCipher::new);
    }

    public BRCryptoCipher() {
        super();
    }

    public BRCryptoCipher(Pointer address) {
        super(address);
    }

    public Optional<byte[]> encrypt(byte[] input) {
        Pointer thisPtr = this.getPointer();

        SizeT length = CryptoLibraryDirect.cryptoCipherEncryptLength(thisPtr, input, new SizeT(input.length));
        int lengthAsInt = Ints.checkedCast(length.longValue());
        if (0 == lengthAsInt) return Optional.absent();

        byte[] output = new byte[lengthAsInt];
        int result = CryptoLibraryDirect.cryptoCipherEncrypt(thisPtr, output, new SizeT(output.length), input, new SizeT(input.length));
        return result == BRCryptoBoolean.CRYPTO_TRUE ? Optional.of(output) : Optional.absent();
    }

    public Optional<byte[]> decrypt(byte[] input) {
        Pointer thisPtr = this.getPointer();

        SizeT length = CryptoLibraryDirect.cryptoCipherDecryptLength(thisPtr, input, new SizeT(input.length));
        int lengthAsInt = Ints.checkedCast(length.longValue());
        if (0 == lengthAsInt) return Optional.absent();

        byte[] output = new byte[lengthAsInt];
        int result = CryptoLibraryDirect.cryptoCipherDecrypt(thisPtr, output, new SizeT(output.length), input, new SizeT(input.length));
        return result == BRCryptoBoolean.CRYPTO_TRUE ? Optional.of(output) : Optional.absent();
    }

    public Optional<byte[]> migrateBRCoreKeyCiphertext(byte[] input) {
        Pointer thisPtr = this.getPointer();

        int lengthAsInt = input.length;
        if (0 == lengthAsInt) return Optional.absent();

        byte[] output = new byte[lengthAsInt];
        int result = CryptoLibraryDirect.cryptoCipherMigrateBRCoreKeyCiphertext(thisPtr, output, new SizeT(output.length), input, new SizeT(input.length));
        return result == BRCryptoBoolean.CRYPTO_TRUE ? Optional.of(output) : Optional.absent();
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoCipherGive(thisPtr);
    }
}
