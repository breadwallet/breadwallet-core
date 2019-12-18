/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.wallet;

import com.breadwallet.crypto.WalletState;

public final class WalletChangedEvent implements WalletEvent {

    private final WalletState oldState;
    private final WalletState newState;

    public WalletChangedEvent(WalletState oldState, WalletState newState) {
        this.oldState = oldState;
        this.newState = newState;
    }

    public WalletState getOldState() {
        return oldState;
    }

    public WalletState getNewState() {
        return newState;
    }

    @Override
    public <T> T accept(WalletEventVisitor<T> visitor) {
        return visitor.visit(this);
    }

    @Override
    public String toString() {
        return "WalletChangedEvent{" +
                "oldState=" + oldState +
                ", newState=" + newState +
                '}';
    }
}
