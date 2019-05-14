package com.breadwallet.crypto.events.system;

import com.breadwallet.crypto.WalletManager;

public class SystemManagerAddedEvent implements SystemEvent {

    private final WalletManager walletManager;

    public SystemManagerAddedEvent(WalletManager walletManager) {
        this.walletManager = walletManager;
    }

    public WalletManager getWalletManager() {
        return walletManager;
    }
}
