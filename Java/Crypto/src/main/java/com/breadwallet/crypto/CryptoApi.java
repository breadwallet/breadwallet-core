/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.events.system.SystemListener;
import com.google.common.base.Optional;

import java.util.Date;
import java.util.concurrent.ExecutorService;

import static com.google.common.base.Preconditions.checkState;

public final class CryptoApi {

    public interface AccountProvider {
        Account create(String phrase, String uids, Date earliestKeyTime);
        Account create(byte[] seed, String uids, Date earliestKeyTime);
    }

    public interface AmountProvider {
        Optional<Amount> create(long value, Unit unit);
        Optional<Amount> create(double value, Unit unit);
        Optional<Amount> create(String value, boolean isNegative, Unit unit);
    }

    public interface SystemProvider {
        System create(ExecutorService listenerExecutor, SystemListener listener, Account account, String path, BlockchainDb query);
    }

    public interface Provider {

        AccountProvider accountProvider();

        AmountProvider amountProvider();

        SystemProvider systemProvider();
    }

    private static Provider provider;

    public static void initialize(Provider provider) {
        checkState(null == CryptoApi.provider);
        CryptoApi.provider = provider;
    }

    /* package */
    static Provider getProvider() {
        checkState(null != CryptoApi.provider);
        return CryptoApi.provider;
    }
}
