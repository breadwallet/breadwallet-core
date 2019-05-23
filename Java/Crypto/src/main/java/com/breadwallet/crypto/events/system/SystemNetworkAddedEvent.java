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
}
