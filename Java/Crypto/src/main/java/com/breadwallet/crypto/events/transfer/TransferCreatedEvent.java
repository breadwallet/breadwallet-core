package com.breadwallet.crypto.events.transfer;

public final class TransferCreatedEvent implements TranferEvent {

    @Override
    public <T> T accept(TransferEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
