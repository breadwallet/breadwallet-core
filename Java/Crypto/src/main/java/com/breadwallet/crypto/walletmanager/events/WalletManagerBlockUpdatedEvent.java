package com.breadwallet.crypto.walletmanager.events;

public final class WalletManagerBlockUpdatedEvent implements WalletManagerEvent {

    private final long height;

    public WalletManagerBlockUpdatedEvent(long height) {
        this.height = height;
    }

    public long getHeight() {
        return height;
    }
}
