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

    static Account createFromPhrase(byte[] phraseUtf8, Date timestamp, String uids) {
        return CryptoApi.getProvider().accountProvider().createFromPhrase(phraseUtf8, timestamp, uids);
    }

    static Account createFromSeed(byte[] seed, Date timestamp, String uids) {
        return CryptoApi.getProvider().accountProvider().createFromSeed(seed, timestamp, uids);
    }

    Date getTimestamp();
}
