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
final class Coder implements com.breadwallet.crypto.Coder {

    // TODO(fix): Implement me!

    /* package */
    static Coder createForAlgorithm(Algorithm algorithm) {
        return null;
    }

    @Override
    public String encode(byte[] source) {
        return null;
    }

    @Override
    public Optional<byte[]> decode(String source) {
        return Optional.absent();
    }
}
