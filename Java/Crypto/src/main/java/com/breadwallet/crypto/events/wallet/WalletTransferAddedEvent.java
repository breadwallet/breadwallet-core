package com.breadwallet.crypto.events.wallet;

import com.breadwallet.crypto.Transfer;

public final class WalletTransferAddedEvent implements WalletEvent {

    private final Transfer transfer;

    public WalletTransferAddedEvent(Transfer transfer) {
        this.transfer = transfer;
    }

    public Transfer getTransfer() {
        return transfer;
    }

    @Override
    public String toString() {
        return "TransferAdded";
    }

    @Override
    public <T> T accept(WalletEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
