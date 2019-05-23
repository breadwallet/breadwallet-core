package com.breadwallet.crypto.events.walletmanager;

import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.System;

public interface WalletManagerListener {

    void handleManagerEvent(System system, WalletManager manager, WalletManagerEvent event);
}
