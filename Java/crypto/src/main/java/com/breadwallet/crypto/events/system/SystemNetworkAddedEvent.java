/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.system;

import com.breadwallet.crypto.Network;

public class SystemNetworkAddedEvent implements SystemEvent {

    private final Network network;

    public SystemNetworkAddedEvent(Network network) {
        this.network = network;
    }

    public Network getNetwork() {
        return network;
    }

    @Override
    public <T> T accept(SystemEventVisitor<T> visitor) {
        return visitor.visit(this);
    }

    @Override
    public String toString() {
        return "SystemNetworkAddedEvent{" +
                "network=" + network +
                '}';
    }
}
