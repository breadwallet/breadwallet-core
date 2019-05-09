package com.breadwallet.crypto.wallet.events;

import com.breadwallet.crypto.transfer.Transfer;

public final class WalletTransferAddedEvent implements WalletEvent {

    private final Transfer transfer;

    public WalletTransferAddedEvent(Transfer transfer) {
        this.transfer = transfer;
    }

    public Transfer getTransfer() {
        return transfer;
    }
}
