/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.primitives.UnsignedLong;

class OwnedBRCryptoNetworkFee implements CoreBRCryptoNetworkFee {

    private final BRCryptoNetworkFee core;

    /* package */
    OwnedBRCryptoNetworkFee(BRCryptoNetworkFee core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoNetworkFeeGive(core);
        }
    }

    @Override
    public UnsignedLong getConfirmationTimeInMilliseconds() {
        return core.getConfirmationTimeInMilliseconds();
    }

    @Override
    public boolean isIdentical(CoreBRCryptoNetworkFee fee) {
        return core.isIdentical(fee);
    }

    @Override
    public BRCryptoNetworkFee asBRCryptoNetworkFee() {
        return core;
    }
}
