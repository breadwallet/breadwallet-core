/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.migration;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;

public final class TransactionBlob {

    public static class Btc {
        public final byte[] bytes;
        public final UnsignedInteger blockHeight;
        public final UnsignedInteger timestamp;

        private Btc(byte[] bytes,
                    UnsignedInteger blockHeight,
                    UnsignedInteger timestamp) {
            this.bytes = bytes.clone();
            this.blockHeight = blockHeight;
            this.timestamp = timestamp;
        }
    }

    public static TransactionBlob BTC(byte[] bytes,
                                      UnsignedInteger blockHeight,
                                      UnsignedInteger timestamp) {
        return new TransactionBlob(new Btc(bytes, blockHeight, timestamp));
    }

    @Nullable
    private final Btc btc;

    private TransactionBlob(Btc btc) {
        this.btc = btc;
    }

    public Optional<Btc> asBtc() {
        return Optional.fromNullable(btc);
    }
}
