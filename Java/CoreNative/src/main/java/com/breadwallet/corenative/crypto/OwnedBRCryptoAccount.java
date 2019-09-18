/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;

import java.util.Date;
import java.util.Objects;

/* package */
class OwnedBRCryptoAccount implements CoreBRCryptoAccount {

    private final BRCryptoAccount core;

    /* package */
    OwnedBRCryptoAccount(BRCryptoAccount core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoAccountGive(core);
        }
    }

    @Override
    public Date getTimestamp() {
        return core.getTimestamp();
    }

    @Override
    public byte[] serialize() {
        return core.serialize();
    }

    @Override
    public boolean validate(byte[] serialization) {
        return core.validate(serialization);
    }

    @Override
    public BRCryptoAccount asBRCryptoAccount() {
        return core;
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoAccount) {
            OwnedBRCryptoAccount that = (OwnedBRCryptoAccount) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoAccount) {
            BRCryptoAccount that = (BRCryptoAccount) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
