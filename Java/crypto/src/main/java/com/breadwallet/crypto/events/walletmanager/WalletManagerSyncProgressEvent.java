/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.walletmanager;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;

import java.util.Date;

public final class WalletManagerSyncProgressEvent implements WalletManagerEvent {

    private final float percentComplete;

    @Nullable
    private final Date timestamp;

    public WalletManagerSyncProgressEvent(float percentComplete, @Nullable Date timestamp) {
        this.percentComplete = percentComplete;
        this.timestamp = timestamp;
    }

    public float getPercentComplete() {
        return percentComplete;
    }

    public Optional<Date> getTimestamp() {
        return Optional.fromNullable(timestamp);
    }

    @Override
    public <T> T accept(WalletManagerEventVisitor<T> visitor) {
        return visitor.visit(this);
    }

    @Override
    public String toString() {
        return "WalletManagerSyncProgressEvent{" +
                "percentComplete=" + percentComplete +
                ", timestamp=" + timestamp +
                '}';
    }
}
