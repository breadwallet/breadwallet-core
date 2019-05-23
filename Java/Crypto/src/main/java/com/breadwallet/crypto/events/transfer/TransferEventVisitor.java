package com.breadwallet.crypto.events.transfer;

public interface TransferEventVisitor<T> {

    T visit(TransferChangedEvent event);

    T visit(TransferConfirmationEvent event);

    T visit(TransferCreatedEvent event);

    T visit(TransferDeletedEvent event);
}
