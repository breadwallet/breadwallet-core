package com.breadwallet.crypto.walletmanager.events;

import com.breadwallet.crypto.wallet.Wallet;

public final class WalletManagerWalletChangedEvent implements WalletManagerEvent {

    private final Wallet wallet;

    public WalletManagerWalletChangedEvent(Wallet wallet) {
        this.wallet = wallet;
    }

    public Wallet getWallet() {
        return wallet;
    }
}
