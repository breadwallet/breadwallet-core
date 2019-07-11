/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.CoreBRCryptoAccount;

import java.util.Date;

/* package */
final class Account implements com.breadwallet.crypto.Account {

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
    /* package */
    static Account createFromPhrase(byte[] phraseUtf8, Date timestamp, String uids) {
        return new Account(CoreBRCryptoAccount.createFromPhrase(phraseUtf8, timestamp), uids);
    }

    /**
     * Create an account using a BIP32 seed.
     *
     * @apiNote The caller should take appropriate security measures, like enclosing this method's call in a
     * try-finally block that wipes the seed value, to ensure that it is purged from memory
     * upon completion.
     *
     * @param seed The 512-bit BIP32 seed value
     * @param timestamp The timestamp of when this account was first created
     * @param uids The unique identifier of this account
     */
    /* package */
    static Account createFromSeed(byte[] seed, Date timestamp, String uids) {
        return new Account(CoreBRCryptoAccount.createFromSeed(seed, timestamp), uids);
    }

    /* package */
    static byte[] deriveSeed(byte[] phraseUtf8) {
        return CoreBRCryptoAccount.deriveSeed(phraseUtf8);
    }

    /* package */
    static Account from(com.breadwallet.crypto.Account account) {
        if (account instanceof Account) {
            return (Account) account;
        }
        throw new IllegalArgumentException("Unsupported account instance");
    }

    private final CoreBRCryptoAccount core;
    private final String uids;

    private Account(CoreBRCryptoAccount core, String uids) {
        this.uids = uids;
        this.core = core;
    }

    @Override
    public Date getTimestamp() {
        return core.getTimestamp();
    }

    /* package */
    CoreBRCryptoAccount getCoreBRCryptoAccount() {
        return core;
    }
}
