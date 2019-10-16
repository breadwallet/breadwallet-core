/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.walletmanager;

public interface WalletManagerEventVisitor<T> {

    T visit(WalletManagerBlockUpdatedEvent event);

    T visit(WalletManagerChangedEvent event);

    T visit(WalletManagerCreatedEvent event);

    T visit(WalletManagerDeletedEvent event);

    T visit(WalletManagerSyncProgressEvent event);

    T visit(WalletManagerSyncStartedEvent event);

    T visit(WalletManagerSyncStoppedEvent event);

    T visit(WalletManagerSyncRecommendedEvent event);

    T visit(WalletManagerWalletAddedEvent event);

    T visit(WalletManagerWalletChangedEvent event);

    T visit(WalletManagerWalletDeletedEvent event);
}
