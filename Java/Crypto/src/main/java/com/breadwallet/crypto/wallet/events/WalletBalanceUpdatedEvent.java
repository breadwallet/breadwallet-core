package com.breadwallet.crypto.wallet.events;

import com.breadwallet.crypto.Amount;

public final class WalletBalanceUpdatedEvent implements WalletEvent {

    public final Amount balance;

    public WalletBalanceUpdatedEvent(Amount balance) {
        this.balance = balance;
    }
}
