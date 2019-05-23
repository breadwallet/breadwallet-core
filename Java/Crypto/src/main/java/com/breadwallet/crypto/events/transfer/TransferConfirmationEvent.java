package com.breadwallet.crypto.events.transfer;

public final class TransferConfirmationEvent implements TranferEvent {

    private final long count;

    public TransferConfirmationEvent(long count) {
        this.count = count;
    }

    public long getCount() {
        return count;
    }

    @Override
    public <T> T accept(TransferEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
