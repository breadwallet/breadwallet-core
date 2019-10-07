/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoFeeBasis extends PointerType {

    public static BRCryptoFeeBasis createOwned(BRCryptoFeeBasis basis) {
        // TODO(fix): Can the use case here (called when parsed out of struct) be replaced by changing struct to
        //            have BRCryptoFeeBasis.OwnedBRCryptoFeeBasis as its field, instead of BRCryptoFeeBasis?
        return new OwnedBRCryptoFeeBasis(basis.getPointer());
    }

    public BRCryptoFeeBasis(Pointer address) {
        super(address);
    }

    public BRCryptoFeeBasis() {
        super();
    }

    public double getCostFactor() {
        return CryptoLibrary.INSTANCE.cryptoFeeBasisGetCostFactor(this);
    }

    public BRCryptoUnit getPricePerCostFactorUnit() {
        return CryptoLibrary.INSTANCE.cryptoFeeBasisGetPricePerCostFactorUnit(this);
    }

    public BRCryptoAmount getPricePerCostFactor() {
        return CryptoLibrary.INSTANCE.cryptoFeeBasisGetPricePerCostFactor(this);
    }

    public Optional<BRCryptoAmount> getFee() {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoFeeBasisGetFee(this));
    }

    public boolean isIdentical(BRCryptoFeeBasis other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoFeeBasisIsIdentical(this, other);
    }

    public static class OwnedBRCryptoFeeBasis extends BRCryptoFeeBasis {

        public OwnedBRCryptoFeeBasis(Pointer address) {
            super(address);
        }

        public OwnedBRCryptoFeeBasis() {
            super();
        }

        @Override
        protected void finalize() {
            if (null != getPointer()) {
                CryptoLibrary.INSTANCE.cryptoFeeBasisGive(this);
            }
        }
    }
}
