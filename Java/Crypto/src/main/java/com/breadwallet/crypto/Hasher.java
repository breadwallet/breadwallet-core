/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;

public interface Hasher {

    enum Algorithm {
        SHA1,
        SHA224,
        SHA256,
        SHA256_2,
        SHA384,
        SHA512,
        SHA3,
        RMD160,
        HASH160,
        KECCAK256,
        MD5
    }

    static Hasher createForAlgorithm(Algorithm algorithm) {
        return CryptoApi.getProvider().hasherProvider().createHasherForAlgorithm(algorithm);
    }

    Optional<byte[]> hash(byte[] data);
}
