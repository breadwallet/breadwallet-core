/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
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
    public CoreBRCryptoAmount getPricePerCostFactor() {
        return CryptoLibrary.INSTANCE.cryptoFeeBasisGetPricePerCostFactor(this);
    }

    @Override
    public Optional<CoreBRCryptoAmount> getFee() {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoFeeBasisGetFee(this));
    }

    @Override
    public boolean isIdentical(CoreBRCryptoFeeBasis core) {
        return getPricePerCostFactorUnit().isIdentical(core.getPricePerCostFactorUnit()) &&
                getPricePerCostFactor().compare(core.getPricePerCostFactor()) == 0 &&
                getCostFactor() == core.getCostFactor();
    }

    @Override
    public BRCryptoFeeBasis asBRCryptoFeeBasis() {
        return this;
    }
}
