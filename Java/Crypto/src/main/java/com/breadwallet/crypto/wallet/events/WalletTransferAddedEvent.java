package com.breadwallet.crypto.wallet.events;

import com.breadwallet.crypto.transfer.Transfer;

public final class WalletTransferAddedEvent implements WalletEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final Transfer transfer;

    public WalletTransferAddedEvent(Transfer transfer) {
        this.transfer = transfer;
    }
}
