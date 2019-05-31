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

// TODO(Abstraction): This class exposes Core data types via the asBtc/asEth methods. If we want a pure
//                    API, we need to expose the raw data, not the core data types (i.e. btcMasterPubKey,
//                    ethMasterPubKey and ethAddressDetail)

// TODO: Should uid be a UUID object? Or should we verify that it is a valid one?
public final class Account {

    private final CoreBRCryptoAccount core;
    private final String uids;

    public static Account createFrom(String phrase, String uids) {
        return new Account(CoreBRCryptoAccount.create(phrase), uids);
    }

    public static Account createFrom(byte[] seed, String uids) {
        return new Account(CoreBRCryptoAccount.createFromSeed(seed), uids);
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
        // TODO: Can we make this part of the ctor or does it need to be mutable?
        core.setTimestamp(timestamp);
    }

    /* package */
    BRMasterPubKey.ByValue asBtc() {
        return core.asBtc();
    }
}
