/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.system;

import com.breadwallet.crypto.Network;

import java.util.ArrayList;
import java.util.List;

public final class SystemDiscoveredNetworksEvent implements SystemEvent {

    private final List<Network> networks;

    public SystemDiscoveredNetworksEvent(List<Network> networks) {
        this.networks = new ArrayList<>(networks);
    }

    public List<Network> getNetworks() {
        return new ArrayList<>(networks);
    }

    @Override
    public <T> T accept(SystemEventVisitor<T> visitor) {
        return visitor.visit(this);
    }

    @Override
    public String toString() {
        return "SystemDiscoveredNetworksEvent{" +
                "networks=" + networks +
                '}';
    }
}
