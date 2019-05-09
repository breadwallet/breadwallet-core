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

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoAccount;
import com.google.common.base.Optional;

public final class Account {

    /* package */ final BRCryptoAccount core;

    public static Optional<Account> createFrom(String phrase) {
        BRCryptoAccount account = CryptoLibrary.INSTANCE.cryptoAccountCreate(phrase);
        if (account == null) {
            return Optional.absent();
        }
        return Optional.of(new Account(account));
    }

    public static Optional<Account> createFrom(byte[] seed) {
        BRCryptoAccount account = CryptoLibrary.INSTANCE.cryptoAccountCreateFromSeedBytes(seed);
        if (account == null) {
            return Optional.absent();
        }
        return Optional.of(new Account(account));
    }

    public static byte[] deriveSeed(String phrase) {
        return CryptoLibrary.INSTANCE.cryptoAccountDeriveSeed(phrase).u8.clone();
    }

    private Account(BRCryptoAccount core) {
        this.core = core;
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
}
