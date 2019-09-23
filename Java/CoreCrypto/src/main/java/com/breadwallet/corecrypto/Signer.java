/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.google.common.base.Optional;

/* package */
final class Signer implements com.breadwallet.crypto.Signer {

    // TODO(fix): Implement me!

    /* package */
    static Signer createForAlgorithm(Algorithm algorithm) {
        return null;
    }

    @Override
    public byte[] sign(byte[] data, com.breadwallet.crypto.Key key) {
        return new byte[0];
    }

    @Override
    public Optional<Key> recover(byte[] data, byte[] signature) {
        return Optional.absent();
    }
}
