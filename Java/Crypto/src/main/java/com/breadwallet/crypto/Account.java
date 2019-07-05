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

    /**
     * Create an account using a BIP32 phrase.
     *
     * @apiNote The caller should take appropriate security measures, like enclosing this method's call in a
     * try-finally block that wipes the phraseUtf8 value, to ensure that it is purged from memory
     * upon completion.
     *
     * @param phraseUtf8 The UTF-8 NFKD normalized BIP32 phrase
     * @param timestamp The timestamp of when this account was first created
     * @param uids The unique identifier of this account
     */
    static Account createFromPhrase(byte[] phraseUtf8, Date timestamp, String uids) {
        return CryptoApi.getProvider().accountProvider().createFromPhrase(phraseUtf8, timestamp, uids);
    }

    /**
     * Create an account using a BIP32 seed.
     *
     * @apiNote The caller should take appropriate security measures, like enclosing this method's call in a
     * try-finally block that wipes the seed value, to ensure that it is purged from memory
     * upon completion.
     *
     * @param seed The 512-bit BIP32 seed
     * @param timestamp The timestamp of when this account was first created
     * @param uids The unique identifier of this account
     */
    static Account createFromSeed(byte[] seed, Date timestamp, String uids) {
        return CryptoApi.getProvider().accountProvider().createFromSeed(seed, timestamp, uids);
    }

    Date getTimestamp();
}
