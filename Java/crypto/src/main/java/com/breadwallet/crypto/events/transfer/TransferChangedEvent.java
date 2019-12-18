/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.events.transfer;

import com.breadwallet.crypto.TransferState;

public final class TransferChangedEvent implements TranferEvent {

    private final TransferState oldState;
    private final TransferState newState;

    public TransferChangedEvent(TransferState oldState, TransferState newState) {
        this.oldState = oldState;
        this.newState = newState;
    }

    public TransferState getOldState() {
        return oldState;
    }

    public TransferState getNewState() {
        return newState;
    }

    @Override
    public <T> T accept(TransferEventVisitor<T> visitor) {
        return visitor.visit(this);
    }

    @Override
    public String toString() {
        return "TransferChangedEvent{" +
                "oldState=" + oldState +
                ", newState=" + newState +
                '}';
    }
}
