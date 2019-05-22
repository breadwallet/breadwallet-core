package com.breadwallet.crypto.events.wallet;

import com.breadwallet.crypto.WalletState;

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

    @Override
    public String toString() {
        return "StateChanged";
    }
}
