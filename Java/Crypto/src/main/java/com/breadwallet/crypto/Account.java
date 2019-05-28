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

import com.breadwallet.crypto.jni.crypto.CoreBRCryptoAccount;
import com.breadwallet.crypto.jni.support.BRMasterPubKey;
import com.google.common.base.Optional;

// TODO: Should uid be a UUID object? Or should we verify that it is a valid one?
public final class Account {

    private final CoreBRCryptoAccount core;
    private final String uids;

    public static Optional<Account> createFrom(String phrase, String uids) {
        CoreBRCryptoAccount account = CoreBRCryptoAccount.create(phrase);
        if (account == null) {
            return Optional.absent();
        }
        return Optional.of(new Account(account, uids));
    }

    public static Optional<Account> createFrom(byte[] seed, String uids) {
        CoreBRCryptoAccount account = CoreBRCryptoAccount.createFromSeed(seed);
        if (account == null) {
            return Optional.absent();
        }
        return Optional.of(new Account(account, uids));
    }

    public static byte[] deriveSeed(String phrase) {
        return CoreBRCryptoAccount.deriveSeed(phrase);
    }

    private Account(CoreBRCryptoAccount core, String uids) {
        this.core = core;
        this.uids = uids;
    }

    public long getTimestamp() {
        return core.getTimestamp();
    }

    public void setTimestamp(long timestamp) {
        core.setTimestamp(timestamp);
    }

    /* package */
    BRMasterPubKey.ByValue asBtc() {
        return core.asBtc();
    }
}
