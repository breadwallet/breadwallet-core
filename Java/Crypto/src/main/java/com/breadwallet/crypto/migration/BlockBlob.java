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

        public final BlockHash hash;
        public final UnsignedInteger height;
        public final UnsignedInteger nonce;
        public final UnsignedInteger target;
        public final UnsignedInteger txCount;
        public final UnsignedInteger version;
        @Nullable
        public final UnsignedInteger timestamp;
        public final byte[] flags;
        public final List<BlockHash> hashes;
        public final BlockHash merkleRoot;
        public final BlockHash prevBlock;

        private Btc(BlockHash hash,
                    UnsignedInteger height,
                    UnsignedInteger nonce,
                    UnsignedInteger target,
                    UnsignedInteger txCount,
                    UnsignedInteger version,
                    @Nullable UnsignedInteger timestamp,
                    byte[] flags,
                    List<BlockHash> hashes,
                    BlockHash merkleRoot,
                    BlockHash prevBlock) {
            this.hash = hash;
            this.height = height;
            this.nonce = nonce;
            this.target = target;
            this.txCount = txCount;
            this.version = version;
            this.timestamp = timestamp;
            this.flags = flags.clone();
            this.hashes = new ArrayList<>(hashes);
            this.merkleRoot = merkleRoot;
            this.prevBlock = prevBlock;
        }
    }

    public static BlockBlob BTC(BlockHash hash,
                                UnsignedInteger height,
                                UnsignedInteger nonce,
                                UnsignedInteger target,
                                UnsignedInteger txCount,
                                UnsignedInteger version,
                                @Nullable UnsignedInteger timestamp,
                                byte[] flags,
                                List<BlockHash> hashes,
                                BlockHash merkleRoot,
                                BlockHash prevBlock) {
        return new BlockBlob(
                new Btc(
                        hash,
                        height,
                        nonce,
                        target,
                        txCount,
                        version,
                        timestamp,
                        flags,
                        hashes,
                        merkleRoot,
                        prevBlock
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
