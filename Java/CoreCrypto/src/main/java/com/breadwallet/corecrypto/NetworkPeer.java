/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 9/19/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.crypto.BRCryptoPeer;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;

import java.util.Objects;

/* package */
final class NetworkPeer implements com.breadwallet.crypto.NetworkPeer {

    /* package */
    static Optional<NetworkPeer> create(com.breadwallet.crypto.Network network, String address, UnsignedInteger port, @Nullable String publicKey) {
        return BRCryptoPeer.create(
                Network.from(network).getCoreBRCryptoNetwork(),
                address,
                port,
                publicKey)
                .transform(NetworkPeer::new);
    }

    /* package */
    static NetworkPeer create(BRCryptoPeer core) {
        return new NetworkPeer(core);
    }

    /* package */
    static NetworkPeer from(com.breadwallet.crypto.NetworkPeer peer) {
        if (peer == null) {
            return null;
        }

        if (peer instanceof NetworkPeer) {
            return (NetworkPeer) peer;
        }

        throw new IllegalArgumentException("Unsupported peer instance");
    }

    private final BRCryptoPeer core;

    private NetworkPeer(BRCryptoPeer core) {
        this.core = core;
    }

    @Override
    public Network getNetwork() {
        return Network.create(core.getNetwork());
    }

    @Override
    public String getAddress() {
        return core.getAddress();
    }

    @Override
    public int getPort() {
        return core.getPort().shortValue();
    }

    @Override
    public Optional<String> getPublicKey() {
        return core.getPublicKey();
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof NetworkPeer)) {
            return false;
        }

        NetworkPeer that = (NetworkPeer) object;
        return core.equals(that.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }

    /* package */
    BRCryptoPeer getBRCryptoPeer() {
        return core;
    }
}
