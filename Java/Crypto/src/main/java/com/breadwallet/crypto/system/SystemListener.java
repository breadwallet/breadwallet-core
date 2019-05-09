package com.breadwallet.crypto.system;

import com.breadwallet.crypto.network.NetworkListener;
import com.breadwallet.crypto.system.events.SystemEvent;
import com.breadwallet.crypto.transfer.TransferListener;
import com.breadwallet.crypto.wallet.WalletListener;
import com.breadwallet.crypto.walletmanager.WalletManagerListener;

public interface SystemListener extends WalletManagerListener, WalletListener, TransferListener, NetworkListener {

    void handleSystemEvent(System system, SystemEvent event);
}
