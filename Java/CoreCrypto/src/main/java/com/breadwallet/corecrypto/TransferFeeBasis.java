/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.BRCryptoFeeBasis;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.util.Objects;

/* package */
class TransferFeeBasis implements com.breadwallet.crypto.TransferFeeBasis {

    /* package */
    static TransferFeeBasis create(BRCryptoFeeBasis core) {
        return new TransferFeeBasis(core);
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

        // TODO(fix): Unchecked get here
        this.feeSupplier = Suppliers.memoize(() -> Amount.create(core.getFee().get()));
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
        // TODO(fix): objects that are equal to each other must return the same hashCode; this implementation doesn;t
        //            meet that requirement
        return Objects.hash(core);
    }

    /* package */
    BRCryptoFeeBasis getCoreBRFeeBasis() {
        return core;
    }
}
