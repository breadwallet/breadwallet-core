/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoCoder;
import com.google.common.base.Optional;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
final class Coder implements com.breadwallet.crypto.Coder {

    /* package */
    static Coder createForAlgorithm(Algorithm algorithm) {
        BRCryptoCoder core = null;
        switch (algorithm) {
            case HEX:
                core = BRCryptoCoder.createHex().orNull();
                break;
            case BASE58:
                core = BRCryptoCoder.createBase58().orNull();
                break;
            case BASE58CHECK:
                core = BRCryptoCoder.createBase58Check().orNull();
                break;
        }

        checkNotNull(core);
        return Coder.create(core);
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
