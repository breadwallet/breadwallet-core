package com.breadwallet.crypto.events.system;

public class SystemCreatedEvent implements SystemEvent {

    @Override
    public <T> T accept(SystemEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
