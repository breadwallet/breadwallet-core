package com.breadwallet.crypto.events.system;

import com.breadwallet.crypto.Network;

public class SystemNetworkAddedEvent implements SystemEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final Network network;

    public SystemNetworkAddedEvent(Network network) {
        this.network = network;
    }
}
