package com.breadwallet.crypto.walletmanager.events;

public final class WalletManagerSyncProgressEvent implements WalletManagerEvent {

    private final double percentComplete;

    public WalletManagerSyncProgressEvent(double percentComplete) {
        this.percentComplete = percentComplete;
    }

    public double getPercentComplete() {
        return percentComplete;
    }
}
