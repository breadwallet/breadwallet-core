package com.breadwallet.crypto.events.network;

import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Network;

public interface NetworkListener {

    void handleNetworkEvent(System system, Network network, NetworkEvent event);
}
