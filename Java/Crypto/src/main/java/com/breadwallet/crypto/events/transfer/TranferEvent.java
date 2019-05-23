package com.breadwallet.crypto.events.transfer;

public interface TranferEvent {

    <T> T accept(TransferEventVisitor<T> visitor);
}
