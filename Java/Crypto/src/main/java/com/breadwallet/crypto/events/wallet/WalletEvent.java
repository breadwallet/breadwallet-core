package com.breadwallet.crypto.events.wallet;

public interface WalletEvent {

    <T> T accept(WalletEventVisitor<T> visitor);
}
