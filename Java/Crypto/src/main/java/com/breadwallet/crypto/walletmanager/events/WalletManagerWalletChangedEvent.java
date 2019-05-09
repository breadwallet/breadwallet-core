package com.breadwallet.crypto.walletmanager.events;

import com.breadwallet.crypto.wallet.Wallet;

public final class WalletManagerWalletChangedEvent implements WalletManagerEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final Wallet wallet;

    public WalletManagerWalletChangedEvent(Wallet wallet) {
        this.wallet = wallet;
    }
}
