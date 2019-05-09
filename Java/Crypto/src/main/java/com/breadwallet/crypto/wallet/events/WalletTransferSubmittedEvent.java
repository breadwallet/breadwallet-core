package com.breadwallet.crypto.wallet.events;

import com.breadwallet.crypto.transfer.Transfer;

public final class WalletTransferSubmittedEvent implements WalletEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final Transfer transfer;

    public WalletTransferSubmittedEvent(Transfer transfer) {
        this.transfer = transfer;
    }
}
