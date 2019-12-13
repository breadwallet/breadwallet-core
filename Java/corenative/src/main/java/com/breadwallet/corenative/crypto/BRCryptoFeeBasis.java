/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoFeeBasis extends PointerType {

    public BRCryptoFeeBasis() {
        super();
    }

    public BRCryptoFeeBasis(Pointer address) {
        super(address);
    }

    public double getCostFactor() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoFeeBasisGetCostFactor(thisPtr);
    }

    public BRCryptoUnit getPricePerCostFactorUnit() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoUnit(CryptoLibraryDirect.cryptoFeeBasisGetPricePerCostFactorUnit(thisPtr));
    }

    public BRCryptoAmount getPricePerCostFactor() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoAmount(CryptoLibraryDirect.cryptoFeeBasisGetPricePerCostFactor(thisPtr));
    }

    public Optional<BRCryptoAmount> getFee() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(CryptoLibraryDirect.cryptoFeeBasisGetFee(thisPtr)).transform(BRCryptoAmount::new);
    }

    public boolean isIdentical(BRCryptoFeeBasis other) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoFeeBasisIsIdentical(thisPtr, other.getPointer());
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoFeeBasisGive(thisPtr);
    }
}
