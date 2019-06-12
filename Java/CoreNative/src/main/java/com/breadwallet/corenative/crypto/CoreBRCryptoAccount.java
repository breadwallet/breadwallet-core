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

public interface CoreBRCryptoAccount {

    static CoreBRCryptoAccount create(String phrase) {
        return new OwnedBRCryptoAccount(CryptoLibrary.INSTANCE.cryptoAccountCreate(phrase));
    }

    static CoreBRCryptoAccount createFromSeed(byte[] seed) {
        return new OwnedBRCryptoAccount(CryptoLibrary.INSTANCE.cryptoAccountCreateFromSeedBytes(seed));
    }

    static byte[] deriveSeed(String phrase) {
        return CryptoLibrary.INSTANCE.cryptoAccountDeriveSeed(phrase).u8.clone();
    }

    Date getEarliestKeyTime();

    void setEarliestKeyTime(Date earliestKeyTime);

    BRCryptoAccount asBRCryptoAccount();
}
