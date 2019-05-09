package com.breadwallet.crypto.walletmanager.events;

public final class WalletManagerSyncProgressEvent implements WalletManagerEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final double percentComplete;

    public WalletManagerSyncProgressEvent(double percentComplete) {
        this.percentComplete = percentComplete;
    }
}
