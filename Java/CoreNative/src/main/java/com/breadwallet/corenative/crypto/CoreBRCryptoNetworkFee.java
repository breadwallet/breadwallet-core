package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.primitives.UnsignedLong;

public interface CoreBRCryptoNetworkFee {

    static CoreBRCryptoNetworkFee create(UnsignedLong timeIntervalInMilliseconds,
                                         CoreBRCryptoAmount pricePerCostFactor,
                                         CoreBRCryptoUnit pricePerCostFactorUnit) {
        return new OwnedBRCryptoNetworkFee(
                CryptoLibrary.INSTANCE.cryptoNetworkFeeCreate(
                        timeIntervalInMilliseconds.longValue(),
                        pricePerCostFactor.asBRCryptoAmount(),
                        pricePerCostFactorUnit.asBRCryptoUnit()));
    }

    UnsignedLong getConfirmationTimeInMilliseconds();

    boolean isIdentical(CoreBRCryptoNetworkFee fee);

    BRCryptoNetworkFee asBRCryptoNetworkFee();
}
