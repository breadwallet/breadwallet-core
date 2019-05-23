package com.breadwallet.crypto.events.wallet;

import com.breadwallet.crypto.TransferFeeBasis;

public final class WalletFeeBasisUpdatedEvent implements WalletEvent {

    private TransferFeeBasis feeBasis;

    public WalletFeeBasisUpdatedEvent(TransferFeeBasis feeBasis) {
        this.feeBasis = feeBasis;
    }

    public TransferFeeBasis getFeeBasis() {
        return feeBasis;
    }

    @Override
    public String toString() {
        return "FeeBasisUpdated";
    }

    @Override
    public <T> T accept(WalletEventVisitor<T> visitor) {
        return visitor.visit(this);
    }
}
