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

public class BRCryptoSigner extends PointerType {

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
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoSignerCreate(alg));
    }

    public BRCryptoSigner(Pointer address) {
        super(address);
    }

    public BRCryptoSigner() {
        super();
    }

    public byte[] sign(byte[] digest, BRCryptoKey key) {
        checkState(32 == digest.length);

        SizeT length = CryptoLibrary.INSTANCE.cryptoSignerSignLength(this, key, digest, new SizeT(digest.length));
        int lengthAsInt = Ints.checkedCast(length.longValue());
        checkState(0 != lengthAsInt);

        byte[] signature = new byte[lengthAsInt];
        CryptoLibrary.INSTANCE.cryptoSignerSign(this, key, signature, new SizeT(signature.length), digest, new SizeT(digest.length));
        return signature;
    }

    public Optional<BRCryptoKey > recover(byte[] digest, byte[] signature) {
        checkState(32 == digest.length);
        return Optional.fromNullable(
                CryptoLibrary.INSTANCE.cryptoSignerRecover(
                        this,
                        digest,
                        new SizeT(digest.length),
                        signature,
                        new SizeT(signature.length)
                )
        );
    }

    public static class OwnedBRCryptoSigner extends BRCryptoSigner {

        public OwnedBRCryptoSigner(Pointer address) {
            super(address);
        }

        public OwnedBRCryptoSigner() {
            super();
        }

        @Override
        protected void finalize() {
            if (null != getPointer()) {
                CryptoLibrary.INSTANCE.cryptoSignerGive(this);
            }
        }
    }
}
