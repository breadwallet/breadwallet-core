package com.breadwallet.crypto.wallet.events;

import com.breadwallet.crypto.wallet.WalletState;

public final class WalletChangedEvent implements WalletEvent {

    private final WalletState oldState;
    private final WalletState newState;

    public WalletChangedEvent(WalletState oldState, WalletState newState) {
        this.oldState = oldState;
        this.newState = newState;
    }

    public WalletState getOldState() {
        return oldState;
    }

    public WalletState getNewState() {
        return newState;
    }
}
