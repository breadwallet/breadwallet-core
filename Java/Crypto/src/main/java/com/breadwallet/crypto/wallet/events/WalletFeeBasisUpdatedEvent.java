package com.breadwallet.crypto.wallet.events;

import com.breadwallet.crypto.transfer.TransferFeeBasis;

public final class WalletFeeBasisUpdatedEvent implements WalletEvent {

    public TransferFeeBasis feeBasis;

    public WalletFeeBasisUpdatedEvent(TransferFeeBasis feeBasis) {
        this.feeBasis = feeBasis;
    }
}
