package com.breadwallet.crypto.events.system;

public interface SystemEventVisitor<T> {

    T visit(SystemCreatedEvent event);

    T visit(SystemManagerAddedEvent event);

    T visit(SystemNetworkAddedEvent event);
}
