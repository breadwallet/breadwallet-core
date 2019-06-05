/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.crypto;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.google.common.primitives.UnsignedInteger;

/* package */
class OwnedBRCryptoUnit implements CoreBRCryptoUnit {

    private final BRCryptoUnit core;

    /* package */
    OwnedBRCryptoUnit(BRCryptoUnit core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoUnitGive(core);
        }
    }

    @Override
    public String getName() {
        return core.getName();
    }

    @Override
    public String getSymbol() {
        return core.getSymbol();
    }

    @Override
    public UnsignedInteger getDecimals() {
        return core.getDecimals();
    }

    @Override
    public boolean isCompatible(CoreBRCryptoUnit other) {
        return core.isCompatible(other);
    }

    @Override
    public boolean hasCurrency(CoreBRCryptoCurrency currency) {
        return core.hasCurrency(currency);
    }

    @Override
    public BRCryptoUnit asBRCryptoUnit() {
        return core;
    }
}
