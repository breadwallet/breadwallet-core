package com.breadwallet.crypto.transfer.events;

public final class TransferConfirmationEvent implements TranferEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final long count;

    public TransferConfirmationEvent(long count) {
        this.count = count;
    }
}
