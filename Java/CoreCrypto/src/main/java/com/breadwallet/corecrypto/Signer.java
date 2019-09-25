/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.BRCryptoSigner;
import com.google.common.base.Optional;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
final class Signer implements com.breadwallet.crypto.Signer {

    /* package */
    static Signer createForAlgorithm(Algorithm algorithm) {
        BRCryptoSigner core = null;
        switch (algorithm) {
            case BASIC_DER:
                core = BRCryptoSigner.createBasicDer().orNull();
                break;
            case BASIC_JOSE:
                core = BRCryptoSigner.createBasicJose().orNull();
                break;
            case COMPACT:
                core = BRCryptoSigner.createCompact().orNull();
                break;
        }

        checkNotNull(core);
        return new Signer(core);
    }

    private final BRCryptoSigner core;

    private Signer(BRCryptoSigner core) {
        this.core = core;
    }

    @Override
    public byte[] sign(byte[] digest, com.breadwallet.crypto.Key key) {
        return core.sign(digest, Key.from(key).getBRCryptoKey());
    }

    @Override
    public Optional<Key> recover(byte[] digest, byte[] signature) {
        return core.recover(digest, signature).transform(Key::create);
    }
}
