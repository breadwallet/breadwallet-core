/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.walletmanager;

import android.support.annotation.Nullable;

public abstract class DefaultWalletManagerEventVisitor<T> implements WalletManagerEventVisitor<T> {

    @Nullable
    public T visit(WalletManagerBlockUpdatedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletManagerChangedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletManagerCreatedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletManagerDeletedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletManagerSyncProgressEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletManagerSyncStartedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletManagerSyncStoppedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletManagerSyncRecommendedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletManagerWalletAddedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletManagerWalletChangedEvent event) {
        return null;
    }

    @Nullable
    public T visit(WalletManagerWalletDeletedEvent event) {
        return null;
    }
}
