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

import java.util.ArrayList;
import java.util.List;

public final class BlockBlob {

    public static class Btc {

        public final byte[] block;
        public final UnsignedInteger height;

        private Btc(byte[] block,
                    UnsignedInteger height) {
            this.block = block;
            this.height = height;
        }
    }

    public static BlockBlob BTC(byte[] block,
                                UnsignedInteger height) {
        return new BlockBlob(
                new Btc(
                        block,
                        height
                )
        );
    }

    @Nullable
    private final Btc btc;

    private BlockBlob(Btc btc) {
        this.btc = btc;
    }

    public Optional<Btc> asBtc() {
        return Optional.fromNullable(btc);
    }
}
