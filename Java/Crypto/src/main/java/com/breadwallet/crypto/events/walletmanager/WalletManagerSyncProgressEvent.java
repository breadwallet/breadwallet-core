package com.breadwallet.crypto.events.walletmanager;

public final class WalletManagerSyncProgressEvent implements WalletManagerEvent {

    private final double percentComplete;

    public WalletManagerSyncProgressEvent(double percentComplete) {
        this.percentComplete = percentComplete;
    }

    public double getPercentComplete() {
        return percentComplete;
    }

    @Override
    public <T> T accept(WalletManagerEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
