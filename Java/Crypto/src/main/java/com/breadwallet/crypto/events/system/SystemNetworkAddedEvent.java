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
}
