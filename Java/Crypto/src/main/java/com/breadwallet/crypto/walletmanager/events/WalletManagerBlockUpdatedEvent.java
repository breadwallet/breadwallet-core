package com.breadwallet.crypto.walletmanager.events;

public final class WalletManagerBlockUpdatedEvent implements WalletManagerEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final long height;

    public WalletManagerBlockUpdatedEvent(long height) {
        this.height = height;
    }
}
