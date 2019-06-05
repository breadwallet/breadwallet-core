/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.crypto;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.breadwallet.crypto.libcrypto.support.BRMasterPubKey;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import java.util.Date;

public class BRCryptoAccount extends PointerType implements CoreBRCryptoAccount {

    public BRCryptoAccount(Pointer address) {
        super(address);
    }

    public BRCryptoAccount() {
        super();
    }

    @Override
    public Date getEarliestKeyTime() {
        return new Date(CryptoLibrary.INSTANCE.cryptoAccountGetTimestamp(this) * 1000);
    }

    @Override
    public void setEarliestKeyTime(Date earliestKeyTime) {
        CryptoLibrary.INSTANCE.cryptoAccountSetTimestamp(this, earliestKeyTime.getTime() / 1000);
    }

    @Override
    public BRMasterPubKey.ByValue asBtc() {
        return CryptoLibrary.INSTANCE.cryptoAccountAsBTC(this);
    }
}
