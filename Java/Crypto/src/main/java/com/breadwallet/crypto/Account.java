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

import java.util.Date;

public interface Account {

    static Account createFrom(String phrase, String uids, Date earliestKeyTime) {
        return CryptoApi.getProvider().accountProvider().create(phrase, uids, earliestKeyTime);
    }

    static Account createFrom(byte[] seed, String uids, Date earliestKeyTime) {
        return CryptoApi.getProvider().accountProvider().create(seed, uids, earliestKeyTime);
    }

    Date getEarliestKeyTime();
}
