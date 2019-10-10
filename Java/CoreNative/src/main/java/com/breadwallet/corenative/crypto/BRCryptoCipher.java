/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.google.common.primitives.Ints;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import static com.google.common.base.Preconditions.checkState;

public class BRCryptoCipher extends PointerType {

    public static Optional<BRCryptoCipher> createAesEcb(byte[] key) {
        return Optional.fromNullable(
                CryptoLibrary.INSTANCE.cryptoCipherCreateForAESECB(key, new SizeT(key.length))
        );
    }

    public static Optional<BRCryptoCipher> createChaCha20Poly1305(BRCryptoKey key, byte[] nonce12, byte[] ad) {
        return Optional.fromNullable(
                CryptoLibrary.INSTANCE.cryptoCipherCreateForChacha20Poly1305(
                        key,
                        nonce12,
                        new SizeT(nonce12.length),
                        ad,
                        new SizeT(ad.length))
        );
    }

    public static Optional<BRCryptoCipher> createPigeon(BRCryptoKey privKey, BRCryptoKey pubKey, byte[] nonce12) {
        return Optional.fromNullable(
                CryptoLibrary.INSTANCE.cryptoCipherCreateForPigeon(
                        privKey,
                        pubKey,
                        nonce12,
                        new SizeT(nonce12.length))
        );
    }

    public BRCryptoCipher(Pointer address) {
        super(address);
    }

    public BRCryptoCipher() {
        super();
    }

    public Optional<byte[]> encrypt(byte[] input) {
        SizeT length = CryptoLibrary.INSTANCE.cryptoCipherEncryptLength(this, input, new SizeT(input.length));
        int lengthAsInt = Ints.checkedCast(length.longValue());
        if (0 == lengthAsInt) return Optional.absent();

        byte[] output = new byte[lengthAsInt];
        int result = CryptoLibrary.INSTANCE.cryptoCipherEncrypt(this, output, new SizeT(output.length), input, new SizeT(input.length));
        return result == BRCryptoBoolean.CRYPTO_TRUE ? Optional.of(output) : Optional.absent();
    }

    public Optional<byte[]> decrypt(byte[] input) {
        SizeT length = CryptoLibrary.INSTANCE.cryptoCipherDecryptLength(this, input, new SizeT(input.length));
        int lengthAsInt = Ints.checkedCast(length.longValue());
        if (0 == lengthAsInt) return Optional.absent();

        byte[] output = new byte[lengthAsInt];
        int result = CryptoLibrary.INSTANCE.cryptoCipherDecrypt(this, output, new SizeT(output.length), input, new SizeT(input.length));
        return result == BRCryptoBoolean.CRYPTO_TRUE ? Optional.of(output) : Optional.absent();
    }

    public void give() {
        if (null != getPointer()) {
            CryptoLibrary.INSTANCE.cryptoCipherGive(this);
        }
    }
}
