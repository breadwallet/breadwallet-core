package com.breadwallet.crypto.events.walletmanager;

public interface WalletManagerEventVisitor<T> {

    T visit(WalletManagerBlockUpdatedEvent event);

    T visit(WalletManagerChangedEvent event);

    T visit(WalletManagerCreatedEvent event);

    T visit(WalletManagerDeletedEvent event);

    T visit(WalletManagerSyncProgressEvent event);

    T visit(WalletManagerSyncStartedEvent event);

    T visit(WalletManagerSyncStoppedEvent event);

    T visit(WalletManagerWalletAddedEvent event);

    T visit(WalletManagerWalletChangedEvent event);

    T visit(WalletManagerWalletDeletedEvent event);
}
