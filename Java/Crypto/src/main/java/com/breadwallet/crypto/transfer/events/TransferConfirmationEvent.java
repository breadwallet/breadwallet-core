package com.breadwallet.crypto.transfer.events;

public final class TransferConfirmationEvent implements TranferEvent {

    private final long count;

    public TransferConfirmationEvent(long count) {
        this.count = count;
    }

    public long getCount() {
        return count;
    }
}
