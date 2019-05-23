package com.breadwallet.crypto.events.transfer;

public final class TransferDeletedEvent implements TranferEvent {

    @Override
    public <T> T accept(TransferEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
