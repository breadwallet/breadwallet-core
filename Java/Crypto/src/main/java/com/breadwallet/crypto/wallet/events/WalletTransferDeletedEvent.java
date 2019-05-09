package com.breadwallet.crypto.wallet.events;

import com.breadwallet.crypto.transfer.Transfer;

public final class WalletTransferDeletedEvent implements WalletEvent {

    // TODO: Part of a large discussion; should these be behind getters?
    public final Transfer transfer;

    public WalletTransferDeletedEvent(Transfer transfer) {
        this.transfer = transfer;
    }
}
