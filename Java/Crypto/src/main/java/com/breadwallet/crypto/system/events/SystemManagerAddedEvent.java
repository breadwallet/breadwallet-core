package com.breadwallet.crypto.system.events;

import com.breadwallet.crypto.walletmanager.WalletManager;

public class SystemManagerAddedEvent implements SystemEvent {

    private final WalletManager walletManager;

    public SystemManagerAddedEvent(WalletManager walletManager) {
        this.walletManager = walletManager;
    }

    public WalletManager getWalletManager() {
        return walletManager;
    }
}
