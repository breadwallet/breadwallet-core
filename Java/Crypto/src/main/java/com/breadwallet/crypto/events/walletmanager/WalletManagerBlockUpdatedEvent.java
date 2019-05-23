package com.breadwallet.crypto.events.walletmanager;

public final class WalletManagerBlockUpdatedEvent implements WalletManagerEvent {

    private final long height;

    public WalletManagerBlockUpdatedEvent(long height) {
        this.height = height;
    }

    public long getHeight() {
        return height;
    }

    @Override
    public <T> T accept(WalletManagerEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
