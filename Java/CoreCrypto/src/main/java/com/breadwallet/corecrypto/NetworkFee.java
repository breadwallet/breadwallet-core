package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.CoreBRCryptoNetworkFee;
import com.google.common.primitives.UnsignedLong;

import java.util.Objects;

/* package */
class NetworkFee implements com.breadwallet.crypto.NetworkFee {

    /* package */
    static NetworkFee create(UnsignedLong timeIntervalInMilliseconds,
                             Amount pricePerCostFactor,
                             Unit pricePerCostFactorUnit) {
        return new NetworkFee(CoreBRCryptoNetworkFee.create(
                timeIntervalInMilliseconds,
                pricePerCostFactor.getCoreBRCryptoAmount(),
                pricePerCostFactorUnit.getCoreBRCryptoUnit()));
    }

    /* package */
    static NetworkFee create(CoreBRCryptoNetworkFee core) {
        return new NetworkFee(core);
    }

    /* package */
    static NetworkFee from(com.breadwallet.crypto.NetworkFee fee) {
        if (fee instanceof NetworkFee) {
            return (NetworkFee) fee;
        }
        throw new IllegalArgumentException("Unsupported network fee instance");
    }

    private final CoreBRCryptoNetworkFee core;

    private NetworkFee(CoreBRCryptoNetworkFee core) {
        this.core = core;
    }

    @Override
    public UnsignedLong getConfirmationTimeInMilliseconds() {
        return core.getConfirmationTimeInMilliseconds();
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
        return Objects.hash(core);
    }

    /* package */
    CoreBRCryptoNetworkFee getCoreBRCryptoNetworkFee() {
        return core;
    }
}
