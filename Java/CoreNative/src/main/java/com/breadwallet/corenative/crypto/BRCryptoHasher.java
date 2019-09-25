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
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.StringArray;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;

import static com.google.common.base.Preconditions.checkState;

public class BRCryptoHasher extends PointerType {

    private static final int CRYPTO_HASHER_SHA1         = 0;
    private static final int CRYPTO_HASHER_SHA224       = 1;
    private static final int CRYPTO_HASHER_SHA256       = 2;
    private static final int CRYPTO_HASHER_SHA256_2     = 3;
    private static final int CRYPTO_HASHER_SHA384       = 4;
    private static final int CRYPTO_HASHER_SHA512       = 5;
    private static final int CRYPTO_HASHER_SHA3         = 6;
    private static final int CRYPTO_HASHER_RMD160       = 7;
    private static final int CRYPTO_HASHER_HASH160      = 8;
    private static final int CRYPTO_HASHER_KECCAK256    = 9;
    private static final int CRYPTO_HASHER_MD5          = 10;

    public static Optional<BRCryptoHasher> createSha1() {
        return create(CRYPTO_HASHER_SHA1);
    }

    public static Optional<BRCryptoHasher> createSha224() {
        return create(CRYPTO_HASHER_SHA224);
    }

    public static Optional<BRCryptoHasher> createSha256() {
        return create(CRYPTO_HASHER_SHA256);
    }

    public static Optional<BRCryptoHasher> createSha256_2() {
        return create(CRYPTO_HASHER_SHA256_2);
    }

    public static Optional<BRCryptoHasher> createSha384() {
        return create(CRYPTO_HASHER_SHA384);
    }

    public static Optional<BRCryptoHasher> createSha512() {
        return create(CRYPTO_HASHER_SHA512);
    }

    public static Optional<BRCryptoHasher> createSha3() {
        return create(CRYPTO_HASHER_SHA3);
    }

    public static Optional<BRCryptoHasher> createRmd160() {
        return create(CRYPTO_HASHER_RMD160);
    }

    public static Optional<BRCryptoHasher> createHash160() {
        return create(CRYPTO_HASHER_HASH160);
    }

    public static Optional<BRCryptoHasher> createKeccak256() {
        return create(CRYPTO_HASHER_KECCAK256);
    }

    public static Optional<BRCryptoHasher> createMd5() {
        return create(CRYPTO_HASHER_MD5);
    }

    private static Optional<BRCryptoHasher> create(int alg) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoHasherCreate(alg));
    }

    public BRCryptoHasher(Pointer address) {
        super(address);
    }

    public BRCryptoHasher() {
        super();
    }

    public byte[] hash(byte[] data) {
        SizeT length = CryptoLibrary.INSTANCE.cryptoHasherLength(this);
        int lengthAsInt = Ints.checkedCast(length.longValue());
        checkState(0 != lengthAsInt);

        byte[] hash = new byte[lengthAsInt];
        int result = CryptoLibrary.INSTANCE.cryptoHasherHash(this, hash, new SizeT(hash.length), data, new SizeT(data.length));
        checkState(result == BRCryptoBoolean.CRYPTO_TRUE);
        return hash;
    }

    public static class OwnedBRCryptoHasher extends BRCryptoHasher {

        public OwnedBRCryptoHasher(Pointer address) {
            super(address);
        }

        public OwnedBRCryptoHasher() {
            super();
        }

        @Override
        protected void finalize() {
            if (null != getPointer()) {
                CryptoLibrary.INSTANCE.cryptoHasherGive(this);
            }
        }
    }
}
