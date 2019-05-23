package com.breadwallet.crypto.events.walletmanager;

import com.breadwallet.crypto.Wallet;

public final class WalletManagerWalletChangedEvent implements WalletManagerEvent {

    private final Wallet wallet;

    public WalletManagerWalletChangedEvent(Wallet wallet) {
        this.wallet = wallet;
    }

    public Wallet getWallet() {
        return wallet;
    }

    @Override
    public <T> T accept(WalletManagerEventVisitor<T> visitor) {
        return visitor.visit(this);
    }

}
