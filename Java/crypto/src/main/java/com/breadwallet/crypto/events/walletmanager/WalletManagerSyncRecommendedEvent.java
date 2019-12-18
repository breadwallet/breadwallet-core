/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/11/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.walletmanager;

import com.breadwallet.crypto.WalletManagerSyncDepth;
import com.breadwallet.crypto.WalletManagerSyncStoppedReason;

public final class WalletManagerSyncRecommendedEvent implements WalletManagerEvent {

    private final WalletManagerSyncDepth depth;

    public WalletManagerSyncRecommendedEvent(WalletManagerSyncDepth depth) {
        this.depth = depth;
    }

    public WalletManagerSyncDepth getDepth() {
        return depth;
    }

    @Override
    public <T> T accept(WalletManagerEventVisitor<T> visitor) {
        return visitor.visit(this);
    }

    @Override
    public String toString() {
        return "WalletManagerSyncRecommendedEvent{" +
                "depth=" + depth +
                '}';
    }
}
