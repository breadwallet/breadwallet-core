/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.support.BRMasterPubKey;

import java.util.Date;

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
    public Date getEarliestKeyTime() {
        return core.getEarliestKeyTime();
    }

    @Override
    public void setEarliestKeyTime(Date earliestKeyTime) {
        core.setEarliestKeyTime(earliestKeyTime);
    }

    @Override
    public BRMasterPubKey.ByValue asBtc() {
        return core.asBtc();
    }
}
