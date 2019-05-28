package com.breadwallet.crypto.events.wallet;

import com.breadwallet.crypto.Transfer;

public final class WalletTransferChangedEvent implements WalletEvent {

    private final Transfer transfer;

    public WalletTransferChangedEvent(Transfer transfer) {
        this.transfer = transfer;
    }

    public Transfer getTransfer() {
        return transfer;
    }

    @Override
    public <T> T accept(WalletEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
