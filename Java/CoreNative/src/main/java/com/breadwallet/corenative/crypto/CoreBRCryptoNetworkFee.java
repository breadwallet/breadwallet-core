/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
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
