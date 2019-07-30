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
import java.util.List;
import java.util.concurrent.ScheduledExecutorService;

import static com.google.common.base.Preconditions.checkState;

public final class CryptoApi {

    public interface AccountProvider {
        Account createFromPhrase(byte[] phraseUtf8, Date timestamp, String uids);
        Optional<Account> createFromSerialization(byte[] serialization, String uids);
        byte[] generatePhrase(List<String> words);
        boolean validatePhrase(byte[] phraseUtf8, List<String> words);
    }

    public interface AmountProvider {
        Optional<Amount> create(long value, Unit unit);
        Optional<Amount> create(double value, Unit unit);
        Optional<Amount> create(String value, boolean isNegative, Unit unit);
    }

    public interface SystemProvider {
        System create(ScheduledExecutorService executor, SystemListener listener, Account account, String path, BlockchainDb query);
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
