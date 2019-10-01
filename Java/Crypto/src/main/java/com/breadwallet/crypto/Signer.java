/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;

public interface Signer {

    enum Algorithm {
        BASIC_DER,
        BASIC_JOSE,
        COMPACT
    }

    static Signer createForAlgorithm(Algorithm algorithm) {
        return CryptoApi.getProvider().signerProvider().createSignerForAlgorithm(algorithm);
    }

    Optional<byte[]> sign(byte[] digest, Key key);

    Optional<? extends Key> recover(byte[] digest, byte[] signature);
}
