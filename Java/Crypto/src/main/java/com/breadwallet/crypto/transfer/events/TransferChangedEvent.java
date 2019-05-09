package com.breadwallet.crypto.transfer.events;

import com.breadwallet.crypto.transfer.TransferState;

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
}
