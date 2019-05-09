package com.breadwallet.crypto.walletmanager;

import com.breadwallet.crypto.system.System;
import com.breadwallet.crypto.walletmanager.events.WalletManagerEvent;

public interface WalletManagerListener {

    void handleWalletEvent(System system, WalletManager manager, WalletManagerEvent event);
}
