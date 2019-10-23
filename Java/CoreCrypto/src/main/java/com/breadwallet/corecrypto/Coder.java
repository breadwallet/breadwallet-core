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
import com.breadwallet.corenative.crypto.BRCryptoCoder;
import com.google.common.base.Optional;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
final class Coder implements com.breadwallet.crypto.Coder {

    @Nullable
    private static final Coder CODER_HEX = BRCryptoCoder.createHex().transform(Coder::create).orNull();

    @Nullable
    private static final Coder CODER_BASE58 = BRCryptoCoder.createBase58().transform(Coder::create).orNull();

    @Nullable
    private static final Coder CODER_BASE58CHECK = BRCryptoCoder.createBase58Check().transform(Coder::create).orNull();

    /* package */
    static Coder createForAlgorithm(Algorithm algorithm) {
        Coder coder = null;

        switch (algorithm) {
            case HEX:
                coder = CODER_HEX;
                break;
            case BASE58:
                coder = CODER_BASE58;
                break;
            case BASE58CHECK:
                coder = CODER_BASE58CHECK;
                break;
        }

        checkNotNull(coder);
        return coder;
    }

    private static Coder create(BRCryptoCoder core) {
        Coder coder = new Coder(core);
        ReferenceCleaner.register(coder, core::give);
        return coder;
    }

    private final BRCryptoCoder core;

    private Coder(BRCryptoCoder core) {
        this.core = core;
    }

    @Override
    public Optional<String> encode(byte[] source) {
        return core.encode(source);
    }

    @Override
    public Optional<byte[]> decode(String source) {
        return core.decode(source);
    }
}
