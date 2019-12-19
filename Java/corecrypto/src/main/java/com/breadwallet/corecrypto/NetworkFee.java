/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;
import com.google.common.primitives.UnsignedLong;

import java.util.Objects;

/* package */
class NetworkFee implements com.breadwallet.crypto.NetworkFee {

    /* package */
    static NetworkFee create(UnsignedLong timeIntervalInMilliseconds,
                             Amount pricePerCostFactor) {
        return NetworkFee.create(
                BRCryptoNetworkFee.create(
                        timeIntervalInMilliseconds,
                        pricePerCostFactor.getCoreBRCryptoAmount(),
                        pricePerCostFactor.getUnit().getCoreBRCryptoUnit()
                )
        );
    }

    /* package */
    static NetworkFee create(BRCryptoNetworkFee core) {
        NetworkFee fee = new NetworkFee(core);
        ReferenceCleaner.register(fee, core::give);
        return fee;
    }

    /* package */
    static NetworkFee from(com.breadwallet.crypto.NetworkFee fee) {
        if (fee == null) {
            return null;
        }

        if (fee instanceof NetworkFee) {
            return (NetworkFee) fee;
        }

        throw new IllegalArgumentException("Unsupported network fee instance");
    }

    private final BRCryptoNetworkFee core;

    private final Supplier<UnsignedLong> confTimeSupplier;

    private NetworkFee(BRCryptoNetworkFee core) {
        this.core = core;

        this.confTimeSupplier = Suppliers.memoize(core::getConfirmationTimeInMilliseconds);
    }

    @Override
    public UnsignedLong getConfirmationTimeInMilliseconds() {
        return confTimeSupplier.get();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }

        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        NetworkFee fee = (NetworkFee) o;
        return core.isIdentical(fee.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(getConfirmationTimeInMilliseconds());
    }

    /* package */
    Amount getPricePerCostFactor() {
        return Amount.create(core.getPricePerCostFactor());
    }

    /* package */
    BRCryptoNetworkFee getCoreBRCryptoNetworkFee() {
        return core;
    }
}
