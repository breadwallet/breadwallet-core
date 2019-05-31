/*
 * Account
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Account;
import com.breadwallet.crypto.libcrypto.crypto.CoreBRCryptoAccount;
import com.breadwallet.crypto.libcrypto.support.BRMasterPubKey;

// TODO(discuss): Should uid be a UUID object? Or should we verify that it is a valid one? Also, why uids and not uuid?
public final class AccountImpl implements Account {

    public static AccountImpl createFrom(String phrase, String uids) {
        return new AccountImpl(CoreBRCryptoAccount.create(phrase), uids);
    }

    public static AccountImpl createFrom(byte[] seed, String uids) {
        return new AccountImpl(CoreBRCryptoAccount.createFromSeed(seed), uids);
    }

    public static byte[] deriveSeed(String phrase) {
        return CoreBRCryptoAccount.deriveSeed(phrase);
    }

    private final CoreBRCryptoAccount core;
    private final String uids;

    private AccountImpl(CoreBRCryptoAccount core, String uids) {
        this.core = core;
        this.uids = uids;
    }

    @Override
    public long getTimestamp() {
        return core.getTimestamp();
    }

    @Override
    public void setTimestamp(long timestamp) {
        // TODO(discuss): Can we make this part of the ctor or does it need to be mutable?
        core.setTimestamp(timestamp);
    }

    @Override
    public BRMasterPubKey.ByValue asBtc() {
        return core.asBtc();
    }
}
