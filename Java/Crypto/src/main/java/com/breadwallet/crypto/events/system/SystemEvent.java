package com.breadwallet.crypto.events.system;

public interface SystemEvent {

    <T> T accept(SystemEventVisitor<T> visitor);
}
