package com.breadwallet.crypto.events.wallet;

public final class WalletCreatedEvent implements WalletEvent {

    @Override
    public <T> T accept(WalletEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
