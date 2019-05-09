package com.breadwallet.crypto.walletmanager.events;

import com.google.common.base.Optional;

public final class WalletManagerSyncStoppedEvent implements WalletManagerEvent {

    private final String error;

    public WalletManagerSyncStoppedEvent(String error) {
        this.error = error;
    }

    public Optional<String> getError() {
        return Optional.fromNullable(error);
    }
}
