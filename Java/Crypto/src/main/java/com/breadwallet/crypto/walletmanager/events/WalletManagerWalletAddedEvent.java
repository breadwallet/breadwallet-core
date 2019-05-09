package com.breadwallet.crypto.walletmanager.events;

import com.breadwallet.crypto.wallet.Wallet;

public final class WalletManagerWalletAddedEvent implements WalletManagerEvent {

    private final Wallet wallet;

    public WalletManagerWalletAddedEvent(Wallet wallet) {
        this.wallet = wallet;
    }

    public Wallet getWallet() {
        return wallet;
    }
}
