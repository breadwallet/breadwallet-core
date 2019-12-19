/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoSigner;
import com.google.common.base.Optional;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
final class Signer implements com.breadwallet.crypto.Signer {

    @Nullable
    private static final Signer SIGNER_BASIC_DER = BRCryptoSigner.createBasicDer().transform(Signer::create).orNull();

    @Nullable
    private static final Signer SIGNER_BASIC_JOSE = BRCryptoSigner.createBasicJose().transform(Signer::create).orNull();

    @Nullable
    private static final Signer SIGNER_COMPACT = BRCryptoSigner.createCompact().transform(Signer::create).orNull();

    /* package */
    static Signer createForAlgorithm(Algorithm algorithm) {
        Signer signer = null;

        switch (algorithm) {
            case BASIC_DER:
                signer = SIGNER_BASIC_DER;
                break;
            case BASIC_JOSE:
                signer = SIGNER_BASIC_JOSE;
                break;
            case COMPACT:
                signer = SIGNER_COMPACT;
                break;
        }

        checkNotNull(signer);
        return signer;
    }

    private static Signer create(BRCryptoSigner core) {
        Signer signer = new Signer(core);
        ReferenceCleaner.register(signer, core::give);
        return signer;
    }

    private final BRCryptoSigner core;

    private Signer(BRCryptoSigner core) {
        this.core = core;
    }

    @Override
    public Optional<byte[]> sign(byte[] digest, com.breadwallet.crypto.Key key) {
        Key cryptoKey = Key.from(key);
        return core.sign(digest, cryptoKey.getBRCryptoKey());
    }

    @Override
    public Optional<Key> recover(byte[] digest, byte[] signature) {
        return core.recover(digest, signature).transform(Key::create);
    }
}
