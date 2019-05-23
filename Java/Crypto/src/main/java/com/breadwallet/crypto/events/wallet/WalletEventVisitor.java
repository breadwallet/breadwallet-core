package com.breadwallet.crypto.events.wallet;

public interface WalletEventVisitor<T> {

    T visit(WalletBalanceUpdatedEvent event);

    T visit(WalletChangedEvent event);

    T visit(WalletCreatedEvent event);

    T visit(WalletDeletedEvent event);

    T visit(WalletFeeBasisUpdatedEvent event);

    T visit(WalletTransferAddedEvent event);

    T visit(WalletTransferChangedEvent event);

    T visit(WalletTransferDeletedEvent event);

    T visit(WalletTransferSubmittedEvent event);
}
