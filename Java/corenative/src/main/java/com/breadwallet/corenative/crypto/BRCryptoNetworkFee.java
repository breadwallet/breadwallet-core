/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoNetworkFee extends PointerType {

    public static BRCryptoNetworkFee create(UnsignedLong timeIntervalInMilliseconds,
                                            BRCryptoAmount pricePerCostFactor,
                                            BRCryptoUnit pricePerCostFactorUnit) {
        return new BRCryptoNetworkFee(
                CryptoLibraryDirect.cryptoNetworkFeeCreate(
                        timeIntervalInMilliseconds.longValue(),
                        pricePerCostFactor.getPointer(),
                        pricePerCostFactorUnit.getPointer()
                )
        );
    }

    public BRCryptoNetworkFee() {
        super();
    }

    public BRCryptoNetworkFee(Pointer address) {
        super(address);
    }

    public UnsignedLong getConfirmationTimeInMilliseconds() {
        Pointer thisPtr = this.getPointer();

        return UnsignedLong.valueOf(CryptoLibraryDirect.cryptoNetworkFeeGetConfirmationTimeInMilliseconds(thisPtr));
    }

    public BRCryptoAmount getPricePerCostFactor() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoAmount(CryptoLibraryDirect.cryptoNetworkFeeGetPricePerCostFactor(thisPtr));
    }

    public boolean isIdentical(BRCryptoNetworkFee other) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoNetworkFeeEqual(thisPtr, other.getPointer());
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoNetworkFeeGive(thisPtr);
    }
}
