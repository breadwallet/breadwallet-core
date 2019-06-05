/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Account;
import com.breadwallet.crypto.libcrypto.crypto.CoreBRCryptoAccount;
import com.breadwallet.crypto.libcrypto.support.BRMasterPubKey;

import java.util.Date;
import java.util.UUID;

/* package */
final class AccountImpl implements Account {

    /* package */
    static AccountImpl createFrom(String phrase, String uids, Date earliestKeyTime) {
        return new AccountImpl(CoreBRCryptoAccount.create(phrase), uids, earliestKeyTime);
    }

    /* package */
    static AccountImpl createFrom(byte[] seed, String uids, Date earliestKeyTime) {
        return new AccountImpl(CoreBRCryptoAccount.createFromSeed(seed), uids, earliestKeyTime);
    }

    /* package */
    static byte[] deriveSeed(String phrase) {
        return CoreBRCryptoAccount.deriveSeed(phrase);
    }

    /* package */
    static AccountImpl from(Account account) {
        if (account instanceof AccountImpl) {
            return (AccountImpl) account;
        }
        throw new IllegalArgumentException("Unsupported account instance");
    }

    private final CoreBRCryptoAccount core;
    private final String uids;

    private AccountImpl(CoreBRCryptoAccount core, String uids, Date earliestKeyTime) {
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
}
