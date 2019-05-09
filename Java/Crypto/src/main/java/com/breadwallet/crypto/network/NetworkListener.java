package com.breadwallet.crypto.network;

import com.breadwallet.crypto.system.System;
import com.breadwallet.crypto.network.events.NetworkEvent;

public interface NetworkListener {

    void handleNetworkEvent(System system, Network network, NetworkEvent networkEvent);
}
