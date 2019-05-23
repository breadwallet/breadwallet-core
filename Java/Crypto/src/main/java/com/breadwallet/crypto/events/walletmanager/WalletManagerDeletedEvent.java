package com.breadwallet.crypto.events.walletmanager;

public final class WalletManagerDeletedEvent implements WalletManagerEvent {

    @Override
    public <T> T accept(WalletManagerEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
