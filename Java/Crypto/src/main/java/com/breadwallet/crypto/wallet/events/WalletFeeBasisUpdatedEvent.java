package com.breadwallet.crypto.wallet.events;

import com.breadwallet.crypto.transfer.TransferFeeBasis;

public final class WalletFeeBasisUpdatedEvent implements WalletEvent {

    private TransferFeeBasis feeBasis;

    public WalletFeeBasisUpdatedEvent(TransferFeeBasis feeBasis) {
        this.feeBasis = feeBasis;
    }

    public TransferFeeBasis getFeeBasis() {
        return feeBasis;
    }
}
