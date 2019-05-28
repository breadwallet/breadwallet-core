package com.breadwallet.crypto.events.wallet;

import com.breadwallet.crypto.Amount;

public final class WalletBalanceUpdatedEvent implements WalletEvent {

    private final Amount balance;

    public WalletBalanceUpdatedEvent(Amount balance) {
        this.balance = balance;
    }

    public Amount getBalance() {
        return balance;
    }

    @Override
    public <T> T accept(WalletEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
