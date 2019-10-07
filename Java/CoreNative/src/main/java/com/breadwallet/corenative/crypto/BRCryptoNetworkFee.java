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
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoNetworkFee extends PointerType {

    public static BRCryptoNetworkFee create(UnsignedLong timeIntervalInMilliseconds,
                                            BRCryptoAmount pricePerCostFactor,
                                            CoreBRCryptoUnit pricePerCostFactorUnit) {
        return CryptoLibrary.INSTANCE.cryptoNetworkFeeCreate(
                        timeIntervalInMilliseconds.longValue(),
                        pricePerCostFactor,
                        pricePerCostFactorUnit.asBRCryptoUnit());
    }

    public BRCryptoNetworkFee(Pointer address) {
        super(address);
    }

    public BRCryptoNetworkFee() {
        super();
    }

    public UnsignedLong getConfirmationTimeInMilliseconds() {
        return UnsignedLong.valueOf(CryptoLibrary.INSTANCE.cryptoNetworkFeeGetConfirmationTimeInMilliseconds(this));
    }

    public boolean isIdentical(BRCryptoNetworkFee other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoNetworkFeeEqual(this, other);
    }

    public static class OwnedBRCryptoNetworkFee extends BRCryptoNetworkFee {

        public OwnedBRCryptoNetworkFee(Pointer address) {
            super(address);
        }

        public OwnedBRCryptoNetworkFee() {
            super();
        }

        @Override
        protected void finalize() {
            if (null != getPointer()) {
                CryptoLibrary.INSTANCE.cryptoNetworkFeeGive(this);
            }
        }
    }
}
