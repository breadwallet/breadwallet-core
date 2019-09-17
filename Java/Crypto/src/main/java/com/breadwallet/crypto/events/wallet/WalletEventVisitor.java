/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
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
