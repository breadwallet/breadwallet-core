/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.wallet;

import android.support.annotation.Nullable;

public abstract class DefaultWalletEventVisitor<T> implements WalletEventVisitor<T> {

    @Nullable
    public T visit(WalletBalanceUpdatedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletChangedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletCreatedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletDeletedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletFeeBasisUpdatedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletTransferAddedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletTransferChangedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletTransferDeletedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletTransferSubmittedEvent event) {
        return null;
    }
}
