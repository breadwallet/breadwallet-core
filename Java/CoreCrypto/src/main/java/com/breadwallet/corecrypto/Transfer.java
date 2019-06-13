/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.CoreBRCryptoTransfer;
import com.breadwallet.crypto.TransferDirection;
import com.breadwallet.crypto.TransferState;
import com.google.common.base.Optional;

import java.util.Objects;
import java.util.concurrent.atomic.AtomicReference;

/* package */
final class Transfer implements com.breadwallet.crypto.Transfer {

    /* package */
    static Transfer create(CoreBRCryptoTransfer transfer, Wallet wallet, Unit unit) {
        return new Transfer(transfer, wallet, unit);
    }

    /* package */
    static Transfer from(com.breadwallet.crypto.Transfer transfer) {
        if (transfer instanceof Transfer) {
            return (Transfer) transfer;
        }
        throw new IllegalArgumentException("Unsupported transfer instance");
    }

    private final CoreBRCryptoTransfer core;
    private final Wallet wallet;
    private final Unit defaultUnit;

    private Transfer(CoreBRCryptoTransfer core, Wallet wallet, Unit defaultUnit) {
        this.core = core;
        this.wallet = wallet;
        this.defaultUnit = defaultUnit;
    }

    @Override
    public Wallet getWallet() {
        return wallet;
    }

    @Override
    public Optional<Address> getSource() {
        return core.getSourceAddress().transform(Address::create);
    }

    @Override
    public Optional<Address> getTarget() {
        return core.getTargetAddress().transform(Address::create);
    }

    @Override
    public Amount getAmount() {
        // TODO(fix): Unchecked get here
        return Amount.create(core.getAmount().get(), wallet.getBaseUnit());
    }

    @Override
     public Amount getAmountDirected() {
        switch (getDirection()) {
            case RECOVERED:
                // TODO(fix): Unchecked get here
                return Amount.create(0L, defaultUnit).get();
            case SENT:
                return getAmount().negate();
            case RECEIVED:
                return getAmount();
            default:
                throw new IllegalStateException("Invalid transfer direction");
        }
    }

    @Override
    public Amount getFee() {
        return Amount.create(core.getFee(), defaultUnit);
    }

    @Override
    public TransferFeeBasis getFeeBasis() {
        return TransferFeeBasis.create(core.getFeeBasis());
    }

    @Override
    public TransferDirection getDirection() {
        return Utilities.transferDirectionFromCrypto(core.getDirection());
    }

    @Override
    public Optional<TransferHash> getHash() {
        return core.getHash().transform(TransferHash::create);
    }

    @Override
    public TransferState getState() {
        // TODO(fix): Deal with memory management for the fee
        return Utilities.transferStateFromCrypto(core.getState());
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof Transfer)) {
            return false;
        }

        Transfer that = (Transfer) object;
        return core.equals(that.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }

    /* package */
    CoreBRCryptoTransfer getCoreBRCryptoTransfer() {
        return core;
    }
}
