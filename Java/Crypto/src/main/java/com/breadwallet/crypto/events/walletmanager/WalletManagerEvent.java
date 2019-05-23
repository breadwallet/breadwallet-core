package com.breadwallet.crypto.events.walletmanager;

public interface WalletManagerEvent {

    <T> T accept(WalletManagerEventVisitor<T> visitor);
}
