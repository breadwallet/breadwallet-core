package com.breadwallet.crypto.events.walletmanager;

import com.breadwallet.crypto.WalletManagerState;

public final class WalletManagerChangedEvent implements WalletManagerEvent {

    private final WalletManagerState oldState;
    private final WalletManagerState newState;

    public WalletManagerChangedEvent(WalletManagerState oldState, WalletManagerState newState) {
        this.oldState = oldState;
        this.newState = newState;
    }

    public WalletManagerState getOldState() {
        return oldState;
    }

    public WalletManagerState getNewState() {
        return newState;
    }
}
