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

import com.breadwallet.crypto.jni.BRMasterPubKey;
import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoAccount;
import com.google.common.base.Optional;

// TODO: Should uid be a UUID object? Or should we verify that it is a valid one?
public final class Account {

    private final BRCryptoAccount core;
    private final String uids;

    public static Optional<Account> createFrom(String phrase, String uids) {
        BRCryptoAccount account = CryptoLibrary.INSTANCE.cryptoAccountCreate(phrase);
        if (account == null) {
            return Optional.absent();
        }
        return Optional.of(new Account(account, uids));
    }

    public static Optional<Account> createFrom(byte[] seed, String uids) {
        BRCryptoAccount account = CryptoLibrary.INSTANCE.cryptoAccountCreateFromSeedBytes(seed);
        if (account == null) {
            return Optional.absent();
        }
        return Optional.of(new Account(account, uids));
    }

    public static byte[] deriveSeed(String phrase) {
        return CryptoLibrary.INSTANCE.cryptoAccountDeriveSeed(phrase).u8.clone();
    }

    private Account(BRCryptoAccount core, String uids) {
        this.core = core;
        this.uids = uids;
    }

    @Override
    protected void finalize() {
        CryptoLibrary.INSTANCE.cryptoAccountGive(core);
    }

    public long getTimestamp() {
        return CryptoLibrary.INSTANCE.cryptoAccountGetTimestamp(core);
    }

    public void setTimestamp(long timestamp) {
        CryptoLibrary.INSTANCE.cryptoAccountSetTimestamp(core, timestamp);
    }

    /* package */
    BRMasterPubKey.ByValue asBtc() {
        return CryptoLibrary.INSTANCE.cryptoAccountAsBTC(core);
    }
}
