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
import com.google.common.primitives.UnsignedLong;

public final class PeerBlob {

    // services value indicating a node carries full blocks, not just headers
    public static final UnsignedLong SERVICES_NODE_NETWORK = UnsignedLong.valueOf(0x01);

    // BIP111: https://github.com/bitcoin/bips/blob/master/bip-0111.mediawiki
    public static final UnsignedLong SERVICES_NODE_BLOOM = UnsignedLong.valueOf(0x01);

    // BIP144: https://github.com/bitcoin/bips/blob/master/bip-0144.mediawiki
    public static final UnsignedLong SERVICES_NODE_WITNESS = UnsignedLong.valueOf(0x08);

    // https://github.com/Bitcoin-UAHF/spec/blob/master/uahf-technical-spec.md
    public static final UnsignedLong SERVICES_NODE_BCASH = UnsignedLong.valueOf(0x20);

    public static class Btc {

        public final UnsignedInteger address;
        public final UnsignedInteger port;
        public final UnsignedLong services;
        @Nullable public final UnsignedInteger timestamp;

        private Btc(UnsignedInteger address,
                    UnsignedInteger port,
                    UnsignedLong services,
                    @Nullable UnsignedInteger timestamp) {
            this.address = address;
            this.port = port;
            this.services = services;
            this.timestamp = timestamp;
        }
    }

    public static PeerBlob BTC(UnsignedInteger address,
                               UnsignedInteger port,
                               UnsignedLong services,
                               @Nullable UnsignedInteger timestamp) {
        return new PeerBlob(new Btc(address, port, services, timestamp));
    }

    @Nullable
    private final Btc btc;

    private PeerBlob(Btc btc) {
        this.btc = btc;
    }

    public Optional<Btc> asBtc() {
        return Optional.fromNullable(btc);
    }
}
