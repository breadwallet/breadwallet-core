/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;

import java.util.Objects;

/* package */
class OwnedBRCryptoHash implements CoreBRCryptoHash {

    private final BRCryptoHash core;

    /* package */
    OwnedBRCryptoHash(BRCryptoHash core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoHashGive(core);
        }
    }

    @Override
    public int getValue() {
        return core.getValue();
    }

    @Override
    public boolean isIdentical(CoreBRCryptoHash other) {
        return core.isIdentical(other);
    }

    @Override
    public BRCryptoHash asBRCryptoHash() {
        return core;
    }

    @Override
    public String toString() {
        return core.toString();
    }

    // TODO(discuss): Do we want to do a value comparison?
    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoHash) {
            OwnedBRCryptoHash that = (OwnedBRCryptoHash) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoHash) {
            BRCryptoHash that = (BRCryptoHash) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
