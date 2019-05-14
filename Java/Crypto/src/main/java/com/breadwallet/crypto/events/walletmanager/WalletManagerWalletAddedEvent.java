package com.breadwallet.crypto.events.walletmanager;

import com.breadwallet.crypto.Wallet;

public final class WalletManagerWalletAddedEvent implements WalletManagerEvent {

    private final Wallet wallet;

    public WalletManagerWalletAddedEvent(Wallet wallet) {
        this.wallet = wallet;
    }

    public Wallet getWallet() {
        return wallet;
    }
}
