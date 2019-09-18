/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;

import java.util.Objects;

/* package */
class OwnedBRCryptoCurrency implements CoreBRCryptoCurrency {

    private final BRCryptoCurrency core;

    /* package */
    OwnedBRCryptoCurrency(BRCryptoCurrency core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoCurrencyGive(core);
        }
    }

    @Override
    public String getUids() {
        return core.getUids();
    }

    @Override
    public String getName() {
        return core.getName();
    }

    @Override
    public String getCode() {
        return core.getCode();
    }

    @Override
    public String getType() {
        return core.getType();
    }

    @Override
    public String getIssuer() {
        return core.getIssuer();
    }

    @Override
    public boolean isIdentical(CoreBRCryptoCurrency other) {
        return core.isIdentical(other);
    }

    @Override
    public BRCryptoCurrency asBRCryptoCurrency() {
        return core;
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoCurrency) {
            OwnedBRCryptoCurrency that = (OwnedBRCryptoCurrency) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoCurrency) {
            BRCryptoCurrency that = (BRCryptoCurrency) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
