package com.breadwallet.crypto.transfer.events;

import com.breadwallet.crypto.transfer.TransferState;

public final class TransferChangedEvent implements TranferEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final TransferState oldState;
    public final TransferState newState;

    public TransferChangedEvent(TransferState oldState, TransferState newState) {
        this.oldState = oldState;
        this.newState = newState;
    }
}
