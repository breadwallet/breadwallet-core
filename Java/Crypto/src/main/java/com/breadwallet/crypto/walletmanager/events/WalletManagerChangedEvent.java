package com.breadwallet.crypto.walletmanager.events;

import com.breadwallet.crypto.walletmanager.WalletManagerState;

public final class WalletManagerChangedEvent implements WalletManagerEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final WalletManagerState oldState;
    public final WalletManagerState newState;

    public WalletManagerChangedEvent(WalletManagerState oldState, WalletManagerState newState) {
        this.oldState = oldState;
        this.newState = newState;
    }
}
