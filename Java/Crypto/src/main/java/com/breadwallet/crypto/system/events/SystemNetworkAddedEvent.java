package com.breadwallet.crypto.system.events;

import com.breadwallet.crypto.network.Network;

public class SystemNetworkAddedEvent implements SystemEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final Network network;

    public SystemNetworkAddedEvent(Network network) {
        this.network = network;
    }
}
