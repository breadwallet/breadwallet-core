package com.breadwallet.crypto.walletmanager.events;

public final class WalletManagerSyncStoppedEvent implements WalletManagerEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final String error;

    public WalletManagerSyncStoppedEvent(String error) {
        this.error = error;
    }
}
