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
class OwnedBRCryptoFeeBasis implements CoreBRCryptoFeeBasis {

    private final BRCryptoFeeBasis core;

    /* package */
    OwnedBRCryptoFeeBasis(BRCryptoFeeBasis core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoFeeBasisGive(core);
        }
    }

    @Override
    public BRCryptoFeeBasis asBRCryptoFeeBasis() {
        return core;
    }

    // TODO(discuss): Do we want to do a value comparison?
    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoFeeBasis) {
            OwnedBRCryptoFeeBasis that = (OwnedBRCryptoFeeBasis) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoFeeBasis) {
            BRCryptoFeeBasis that = (BRCryptoFeeBasis) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
