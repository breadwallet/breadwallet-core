package com.breadwallet.crypto.system.events;

import com.breadwallet.crypto.walletmanager.WalletManager;

public class SystemManagerAddedEvent implements SystemEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final WalletManager walletManager;

    public SystemManagerAddedEvent(WalletManager walletManager) {
        this.walletManager = walletManager;
    }
}
