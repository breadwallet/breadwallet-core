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
}
