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

import static com.google.common.base.Preconditions.checkState;

public class BRCryptoSigner extends PointerType {

    // these must mirror BRCryptoCoderType's enum values
    private static final int CRYPTO_SIGNER_BASIC_DER  = 0;
    private static final int CRYPTO_SIGNER_BASIC_JOSE = 1;
    private static final int CRYPTO_SIGNER_COMPACT    = 2;

    public static Optional<BRCryptoSigner> createBasicDer() {
        return create(CRYPTO_SIGNER_BASIC_DER);
    }

    public static Optional<BRCryptoSigner> createBasicJose() {
        return create(CRYPTO_SIGNER_BASIC_JOSE);
    }

    public static Optional<BRCryptoSigner> createCompact() {
        return create(CRYPTO_SIGNER_COMPACT);
    }

    private static Optional<BRCryptoSigner> create(int alg) {
        return Optional.fromNullable(CryptoLibraryDirect.cryptoSignerCreate(alg)).transform(BRCryptoSigner::new);
    }

    public BRCryptoSigner() {
        super();
    }

    public BRCryptoSigner(Pointer address) {
        super(address);
    }

    public Optional<byte[]> sign(byte[] digest, BRCryptoKey key) {
        checkState(32 == digest.length);
        Pointer thisPtr = this.getPointer();
        Pointer keyPtr = key.getPointer();

        SizeT length = CryptoLibraryDirect.cryptoSignerSignLength(thisPtr, keyPtr, digest, new SizeT(digest.length));
        int lengthAsInt = Ints.checkedCast(length.longValue());
        if (0 == lengthAsInt) return Optional.absent();

        byte[] signature = new byte[lengthAsInt];
        int result = CryptoLibraryDirect.cryptoSignerSign(thisPtr, keyPtr, signature, new SizeT(signature.length), digest, new SizeT(digest.length));
        return result == BRCryptoBoolean.CRYPTO_TRUE ? Optional.of(signature) : Optional.absent();
    }

    public Optional<BRCryptoKey > recover(byte[] digest, byte[] signature) {
        checkState(32 == digest.length);
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoSignerRecover(
                        thisPtr,
                        digest,
                        new SizeT(digest.length),
                        signature,
                        new SizeT(signature.length)
                )
        ).transform(BRCryptoKey::new);
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoSignerGive(thisPtr);
    }
}
