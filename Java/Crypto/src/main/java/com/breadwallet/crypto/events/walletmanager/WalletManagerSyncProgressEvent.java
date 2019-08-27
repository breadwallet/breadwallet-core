/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.walletmanager;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;

import java.util.Date;

public final class WalletManagerSyncProgressEvent implements WalletManagerEvent {

    private final double percentComplete;

    @Nullable
    private final Date timestamp;

    public WalletManagerSyncProgressEvent(double percentComplete, @Nullable Date timestamp) {
        this.percentComplete = percentComplete;
        this.timestamp = timestamp;
    }

    public double getPercentComplete() {
        return percentComplete;
    }

    public Optional<Date> getTimestamp() {
        return Optional.fromNullable(timestamp);
    }

    @Override
    public <T> T accept(WalletManagerEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
