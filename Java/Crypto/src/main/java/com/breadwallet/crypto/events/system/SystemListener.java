package com.breadwallet.crypto.events.system;

import com.breadwallet.crypto.events.network.NetworkListener;
import com.breadwallet.crypto.events.transfer.TransferListener;
import com.breadwallet.crypto.events.wallet.WalletListener;
import com.breadwallet.crypto.events.walletmanager.WalletManagerListener;
import com.breadwallet.crypto.System;

public interface SystemListener extends WalletManagerListener, WalletListener, TransferListener, NetworkListener {

    void handleSystemEvent(System system, SystemEvent event);
}
