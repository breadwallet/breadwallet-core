package com.breadwallet.crypto.walletmanager.events;

import com.breadwallet.crypto.wallet.Wallet;

public final class WalletManagerWalletAddedEvent implements WalletManagerEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final Wallet wallet;

    public WalletManagerWalletAddedEvent(Wallet wallet) {
        this.wallet = wallet;
    }
}
