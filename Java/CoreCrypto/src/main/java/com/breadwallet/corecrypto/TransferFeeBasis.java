package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.CoreBRCryptoFeeBasis;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.util.Objects;

/* package */
class TransferFeeBasis implements com.breadwallet.crypto.TransferFeeBasis {

    /* package */
    static TransferFeeBasis create(CoreBRCryptoFeeBasis core) {
        return new TransferFeeBasis(core);
    }

    /* package */
    static TransferFeeBasis from(com.breadwallet.crypto.TransferFeeBasis feeBasis) {
        if (feeBasis instanceof TransferFeeBasis) {
            return (TransferFeeBasis) feeBasis;
        }
        throw new IllegalArgumentException("Unsupported fee basis instance");
    }

    private final CoreBRCryptoFeeBasis core;

    private final Unit unit;
    private final Supplier<Amount> feeSupplier;
    private final Supplier<Double> costFactorSupplier;
    private final Supplier<Amount> pricePerCostFactorSupplier;

    private TransferFeeBasis(CoreBRCryptoFeeBasis core) {
        this.core = core;

        this.unit= Unit.create(core.getPricePerCostFactorUnit());
        this.costFactorSupplier = Suppliers.memoize(core::getCostFactor);
        this.pricePerCostFactorSupplier = Suppliers.memoize(() -> Amount.create(core.getPricePerCostFactor()));

        // TODO(fix): Unchecked get here
        this.feeSupplier = Suppliers.memoize(() -> Amount.create(core.getFee().get()));
    }

    @Override
    public Unit getUnit() {
        return unit;
    }

    @Override
    public Currency getCurrency() {
        return unit.getCurrency();
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
        return Objects.hash(core);
    }

    /* package */
    CoreBRCryptoFeeBasis getCoreBRFeeBasis() {
        return core;
    }
}
