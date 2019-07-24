package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.CoreBRCryptoFeeBasis;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

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
        this.pricePerCostFactorSupplier = Suppliers.memoize(() -> Amount.create(core.getPricePerCostFactor(), unit));

        // TODO(fix): Unchecked get here
        this.feeSupplier = Suppliers.memoize(() -> Amount.create(core.getFee().get(), unit));
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

    /* package */
    CoreBRCryptoFeeBasis getCoreBRFeeBasis() {
        return core;
    }
}
