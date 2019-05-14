package com.breadwallet.crypto.events.walletmanager;

import com.breadwallet.crypto.Wallet;

public final class WalletManagerWalletDeletedEvent implements WalletManagerEvent {

    private final Wallet wallet;

    public WalletManagerWalletDeletedEvent(Wallet wallet) {
        this.wallet = wallet;
    }

    public Wallet getWallet() {
        return wallet;
    }
}
