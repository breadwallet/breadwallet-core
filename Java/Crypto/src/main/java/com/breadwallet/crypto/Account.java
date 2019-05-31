/*
 * Account
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.implj.AccountImpl;
import com.breadwallet.crypto.libcrypto.support.BRMasterPubKey;

// TODO(discuss): This class exposes Core data types via the asBtc/asEth methods. If we want a pure
//                API, we need to expose the raw data, not the core data types (i.e. btcMasterPubKey,
//                ethMasterPubKey and ethAddressDetail)

public interface Account {

    // TODO(discuss): These could be proper factories

    static Account createFrom(String phrase, String uids) {
        return AccountImpl.createFrom(phrase, uids);
    }

    static Account createFrom(byte[] seed, String uids) {
        return AccountImpl.createFrom(seed, uids);
    }

    long getTimestamp();

    void setTimestamp(long timestamp);

    BRMasterPubKey.ByValue asBtc();
}
