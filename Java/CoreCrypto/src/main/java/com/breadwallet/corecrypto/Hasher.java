/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoHasher;
import com.google.common.base.Optional;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
final class Hasher implements com.breadwallet.crypto.Hasher {

    /* package */
    static Hasher createForAlgorithm(Algorithm algorithm) {
        BRCryptoHasher core = null;
        switch (algorithm) {
            case SHA1:
                core = BRCryptoHasher.createSha1().orNull();
                break;
            case SHA224:
                core = BRCryptoHasher.createSha224().orNull();
                break;
            case SHA256:
                core = BRCryptoHasher.createSha256().orNull();
                break;
            case SHA256_2:
                core = BRCryptoHasher.createSha256_2().orNull();
                break;
            case SHA384:
                core = BRCryptoHasher.createSha384().orNull();
                break;
            case SHA512:
                core = BRCryptoHasher.createSha512().orNull();
                break;
            case SHA3:
                core = BRCryptoHasher.createSha3().orNull();
                break;
            case RMD160:
                core = BRCryptoHasher.createRmd160().orNull();
                break;
            case HASH160:
                core = BRCryptoHasher.createHash160().orNull();
                break;
            case KECCAK256:
                core = BRCryptoHasher.createKeccak256().orNull();
                break;
            case MD5:
                core = BRCryptoHasher.createMd5().orNull();
                break;
        }

        checkNotNull(core);
        return Hasher.create(core);
    }

    private static Hasher create(BRCryptoHasher core) {
        Hasher hasher = new Hasher(core);
        ReferenceCleaner.register(hasher, core::give);
        return hasher;
    }

    private final BRCryptoHasher core;

    private Hasher(BRCryptoHasher core) {
        this.core = core;
    }

    @Override
    public Optional<byte[]> hash(byte[] data) {
        return core.hash(data);
    }
}
