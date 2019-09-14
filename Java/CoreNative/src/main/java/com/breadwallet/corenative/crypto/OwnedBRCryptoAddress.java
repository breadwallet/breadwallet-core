/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;

import java.util.Objects;

/* package */
class OwnedBRCryptoAddress implements CoreBRCryptoAddress {

    private final BRCryptoAddress core;

    /* package */
    OwnedBRCryptoAddress(BRCryptoAddress core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoAddressGive(core);
        }
    }

    @Override
    public boolean isIdentical(CoreBRCryptoAddress address) {
        return core.isIdentical(address);
    }

    @Override
    public String toString() {
        return core.toString();
    }

    @Override
    public BRCryptoAddress asBRCryptoAddress() {
        return core;
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoAddress) {
            OwnedBRCryptoAddress that = (OwnedBRCryptoAddress) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoAddress) {
            BRCryptoAddress that = (BRCryptoAddress) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
