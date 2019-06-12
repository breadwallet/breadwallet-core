/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.crypto.CoreBRCryptoAccount;
import com.breadwallet.corenative.support.BRMasterPubKey;

import java.util.Date;

/* package */
final class Account implements com.breadwallet.crypto.Account {

    /* package */
    static Account create(String phrase, String uids, Date earliestKeyTime) {
        return new Account(CoreBRCryptoAccount.create(phrase), uids, earliestKeyTime);
    }

    /* package */
    static Account create(byte[] seed, String uids, Date earliestKeyTime) {
        return new Account(CoreBRCryptoAccount.createFromSeed(seed), uids, earliestKeyTime);
    }

    /* package */
    static byte[] deriveSeed(String phrase) {
        return CoreBRCryptoAccount.deriveSeed(phrase);
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

    private Account(CoreBRCryptoAccount core, String uids, Date earliestKeyTime) {
        this.uids = uids;
        this.core = core;
        this.core.setEarliestKeyTime(earliestKeyTime);
    }

    @Override
    public Date getEarliestKeyTime() {
        return core.getEarliestKeyTime();
    }

    /* package */
    BRMasterPubKey.ByValue asBtc() {
        return core.asBtc();
    }

    /* package */
    CoreBRCryptoAccount getCoreBRCryptoAccount() {
        return core;
    }
}
