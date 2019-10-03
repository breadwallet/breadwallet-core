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

    public static class Btc {
        @Nullable
        public final UnsignedInteger timestamp;
        public final UnsignedInteger address;
        public final UnsignedInteger port;
        public final UnsignedLong services;

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
