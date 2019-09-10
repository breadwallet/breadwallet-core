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
}
