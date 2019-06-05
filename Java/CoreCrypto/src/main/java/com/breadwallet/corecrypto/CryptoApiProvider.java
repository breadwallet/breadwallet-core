/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.CryptoApi;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.events.system.SystemListener;
import com.google.common.base.Optional;

import java.util.Date;
import java.util.concurrent.ExecutorService;

public final class CryptoApiProvider implements CryptoApi.Provider {

    private static final CryptoApiProvider INSTANCE = new CryptoApiProvider();

    public static CryptoApiProvider getInstance() {
        return INSTANCE;
    }

    private static final CryptoApi.AccountProvider accountProvider = new CryptoApi.AccountProvider() {

        @Override
        public com.breadwallet.crypto.Account create(String phrase, String uids, Date earliestKeyTime) {
            return Account.createFrom(phrase, uids, earliestKeyTime);
        }

        @Override
        public com.breadwallet.crypto.Account create(byte[] seed, String uids, Date earliestKeyTime) {
            return Account.createFrom(seed, uids, earliestKeyTime);
        }
    };

    private static final CryptoApi.AmountProvider amountProvider = new CryptoApi.AmountProvider() {
        @Override
        public Optional<com.breadwallet.crypto.Amount> create(long value, Unit unit) {
            return Amount.create(value, unit).transform(a -> a);
        }

        @Override
        public Optional<com.breadwallet.crypto.Amount> create(double value, Unit unit) {
            return Amount.create(value, unit).transform(a -> a);
        }

        @Override
        public Optional<com.breadwallet.crypto.Amount> create(String value, boolean isNegative, Unit unit) {
            return Amount.create(value, isNegative, unit).transform(a -> a);
        }
    };

    private static final CryptoApi.SystemProvider systemProvider = new CryptoApi.SystemProvider() {

        @Override
        public com.breadwallet.crypto.System create(ExecutorService listenerExecutor, SystemListener listener, com.breadwallet.crypto.Account account, String path, BlockchainDb query) {
            return System.create(listenerExecutor, listener, account, path, query);
        }
    };

    private CryptoApiProvider() {

    }

    @Override
    public CryptoApi.AccountProvider accountProvider() {
        return accountProvider;
    }

    @Override
    public CryptoApi.AmountProvider amountProvider() {
        return amountProvider;
    }

    @Override
    public CryptoApi.SystemProvider systemProvider() {
        return systemProvider;
    }
}
