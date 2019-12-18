/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.walletmanager;

import com.breadwallet.crypto.WalletManagerState;

public final class WalletManagerChangedEvent implements WalletManagerEvent {

    private final WalletManagerState oldState;
    private final WalletManagerState newState;

    public WalletManagerChangedEvent(WalletManagerState oldState, WalletManagerState newState) {
        this.oldState = oldState;
        this.newState = newState;
    }

    public WalletManagerState getOldState() {
        return oldState;
    }

    public WalletManagerState getNewState() {
        return newState;
    }

    @Override
    public <T> T accept(WalletManagerEventVisitor<T> visitor) {
        return visitor.visit(this);
    }

    @Override
    public String toString() {
        return "WalletManagerChangedEvent{" +
                "oldState=" + oldState +
                ", newState=" + newState +
                '}';
    }
}
