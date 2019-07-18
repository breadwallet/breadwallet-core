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
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.util.Objects;

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

    private final Supplier<Optional<Address>> sourceSupplier;
    private final Supplier<Optional<Address>> targetSupplier;
    private final Supplier<Amount> amountSupplier;
    private final Supplier<Amount> directedSupplier;
    private final Supplier<Amount> feeSupplier;
    private final Supplier<TransferFeeBasis> feeBasisSupplier;
    private final Supplier<TransferDirection> directionSupplier;
    private final Supplier<Optional<TransferHash>> hashSupplier;

    private Transfer(CoreBRCryptoTransfer core, Wallet wallet, Unit unit) {
        this.core = core;
        this.wallet = wallet;

        this.sourceSupplier = Suppliers.memoize(() -> core.getSourceAddress().transform(Address::create));
        this.targetSupplier = Suppliers.memoize(() -> core.getTargetAddress().transform(Address::create));
        this.amountSupplier = Suppliers.memoize(() -> Amount.create(core.getAmount(), unit));
        this.directedSupplier = Suppliers.memoize(() -> Amount.create(core.getAmountDirected(), unit));
        this.feeSupplier = Suppliers.memoize(() -> Amount.create(core.getFee(), wallet.getWalletManager().getDefaultUnit()));
        this.feeBasisSupplier = Suppliers.memoize(() -> TransferFeeBasis.create(core.getFeeBasis()));
        this.directionSupplier = Suppliers.memoize(() -> Utilities.transferDirectionFromCrypto(core.getDirection()));
        this.hashSupplier = Suppliers.memoize(() -> core.getHash().transform(TransferHash::create));
    }

    @Override
    public Wallet getWallet() {
        return wallet;
    }

    @Override
    public Optional<Address> getSource() {
        return sourceSupplier.get();
    }

    @Override
    public Optional<Address> getTarget() {
        return targetSupplier.get();
    }

    @Override
    public Amount getAmount() {
        return amountSupplier.get();
    }

    @Override
     public Amount getAmountDirected() {
        return directedSupplier.get();
    }

    @Override
    public Amount getFee() {
        return feeSupplier.get();
    }

    @Override
    public TransferFeeBasis getFeeBasis() {
        return feeBasisSupplier.get();
    }

    @Override
    public TransferDirection getDirection() {
        return directionSupplier.get();
    }

    @Override
    public Optional<TransferHash> getHash() {
        return hashSupplier.get();
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
