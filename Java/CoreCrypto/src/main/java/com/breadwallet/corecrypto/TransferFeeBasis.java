/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoAmount;
import com.breadwallet.corenative.crypto.BRCryptoFeeBasis;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.util.Objects;

import static com.google.common.base.Preconditions.checkState;

/* package */
class TransferFeeBasis implements com.breadwallet.crypto.TransferFeeBasis {

    /* package */
    static TransferFeeBasis create(BRCryptoFeeBasis core) {
        TransferFeeBasis feeBasis = new TransferFeeBasis(core);
        ReferenceCleaner.register(feeBasis, core::give);
        return feeBasis;
    }

    /* package */
    static TransferFeeBasis from(com.breadwallet.crypto.TransferFeeBasis feeBasis) {
        if (feeBasis == null) {
            return null;
        }

        if (feeBasis instanceof TransferFeeBasis) {
            return (TransferFeeBasis) feeBasis;
        }

        throw new IllegalArgumentException("Unsupported fee basis instance");
    }

    private final BRCryptoFeeBasis core;

    private final Supplier<Unit> unitSupplier;
    private final Supplier<Currency> currencySupplier;
    private final Supplier<Amount> feeSupplier;
    private final Supplier<Double> costFactorSupplier;
    private final Supplier<Amount> pricePerCostFactorSupplier;

    private TransferFeeBasis(BRCryptoFeeBasis core) {
        this.core = core;

        this.unitSupplier = Suppliers.memoize(() -> Unit.create(core.getPricePerCostFactorUnit()));
        this.currencySupplier = Suppliers.memoize(() -> getUnit().getCurrency());
        this.costFactorSupplier = Suppliers.memoize(core::getCostFactor);
        this.pricePerCostFactorSupplier = Suppliers.memoize(() -> Amount.create(core.getPricePerCostFactor()));

        this.feeSupplier = Suppliers.memoize(() -> {
            Optional<BRCryptoAmount> maybeAmount = core.getFee();
            checkState(maybeAmount.isPresent());
            return Amount.create(maybeAmount.get());
        });
    }

    @Override
    public Unit getUnit() {
        return unitSupplier.get();
    }

    @Override
    public Currency getCurrency() {
        return currencySupplier.get();
    }

    @Override
    public Amount getPricePerCostFactor() {
        return pricePerCostFactorSupplier.get();
    }

    @Override
    public double getCostFactor() {
        return costFactorSupplier.get();
    }

    @Override
    public Amount getFee() {
        return feeSupplier.get();
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof TransferFeeBasis)) {
            return false;
        }

        TransferFeeBasis feeBasis = (TransferFeeBasis) object;
        return core.isIdentical(feeBasis.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(getFee());
    }

    /* package */
    BRCryptoFeeBasis getCoreBRFeeBasis() {
        return core;
    }
}
