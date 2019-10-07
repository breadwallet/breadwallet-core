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

public class BRCryptoFeeBasis extends PointerType implements CoreBRCryptoFeeBasis {

    public BRCryptoFeeBasis(Pointer address) {
        super(address);
    }

    public BRCryptoFeeBasis() {
        super();
    }

    @Override
    public double getCostFactor() {
        return CryptoLibrary.INSTANCE.cryptoFeeBasisGetCostFactor(this);
    }

    @Override
    public CoreBRCryptoUnit getPricePerCostFactorUnit() {
        return CryptoLibrary.INSTANCE.cryptoFeeBasisGetPricePerCostFactorUnit(this);
    }

    @Override
    public BRCryptoAmount getPricePerCostFactor() {
        return CryptoLibrary.INSTANCE.cryptoFeeBasisGetPricePerCostFactor(this);
    }

    @Override
    public Optional<BRCryptoAmount> getFee() {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoFeeBasisGetFee(this));
    }

    @Override
    public boolean isIdentical(CoreBRCryptoFeeBasis other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoFeeBasisIsIdentical(this, other.asBRCryptoFeeBasis());
    }

    @Override
    public BRCryptoFeeBasis asBRCryptoFeeBasis() {
        return this;
    }
}
