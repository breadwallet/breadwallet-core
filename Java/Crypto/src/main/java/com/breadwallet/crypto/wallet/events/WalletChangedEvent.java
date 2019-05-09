package com.breadwallet.crypto.wallet.events;

import com.breadwallet.crypto.wallet.WalletState;

public final class WalletChangedEvent implements WalletEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final WalletState oldState;
    public final WalletState newState;

    public WalletChangedEvent(WalletState oldState, WalletState newState) {
        this.oldState = oldState;
        this.newState = newState;
    }
}
