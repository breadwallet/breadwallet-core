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

public class BRCryptoHasher extends PointerType {

    // these must mirror BRCryptoHasherType's enum values
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
        return Optional.fromNullable(CryptoLibraryDirect.cryptoHasherCreate(alg)).transform(BRCryptoHasher::new);
    }

    public BRCryptoHasher() {
        super();
    }

    public BRCryptoHasher(Pointer address) {
        super(address);
    }

    public Optional<byte[]> hash(byte[] data) {
        Pointer thisPtr = this.getPointer();

        SizeT length = CryptoLibraryDirect.cryptoHasherLength(thisPtr);
        int lengthAsInt = Ints.checkedCast(length.longValue());
        if (0 == lengthAsInt) return Optional.absent();

        byte[] hash = new byte[lengthAsInt];
        int result = CryptoLibraryDirect.cryptoHasherHash(thisPtr, hash, new SizeT(hash.length), data, new SizeT(data.length));
        return result == BRCryptoBoolean.CRYPTO_TRUE ? Optional.of(hash) : Optional.absent();
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoHasherGive(thisPtr);
    }
}
