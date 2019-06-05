/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.Account;
import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.CryptoApi;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.events.system.SystemListener;
import com.google.common.base.Optional;

import java.util.Date;
import java.util.concurrent.ExecutorService;

public final class CryptoApiImpl implements CryptoApi.Provider {

    private static final CryptoApiImpl INSTANCE = new CryptoApiImpl();

    public static CryptoApiImpl getInstance() {
        return INSTANCE;
    }

    private static final CryptoApi.AccountProvider accountProvider = new CryptoApi.AccountProvider() {

        @Override
        public Account create(String phrase, String uids, Date earliestKeyTime) {
            return AccountImpl.createFrom(phrase, uids, earliestKeyTime);
        }

        @Override
        public Account create(byte[] seed, String uids, Date earliestKeyTime) {
            return AccountImpl.createFrom(seed, uids, earliestKeyTime);
        }
    };

    private static final CryptoApi.AmountProvider amountProvider = new CryptoApi.AmountProvider() {
        @Override
        public Optional<Amount> create(long value, Unit unit) {
            return AmountImpl.create(value, unit);
        }

        @Override
        public Optional<Amount> create(double value, Unit unit) {
            return AmountImpl.create(value, unit);
        }

        @Override
        public Optional<Amount> create(String value, boolean isNegative, Unit unit) {
            return AmountImpl.create(value, isNegative, unit);
        }
    };

    private static final CryptoApi.SystemProvider systemProvider = new CryptoApi.SystemProvider() {

        @Override
        public System create(ExecutorService listenerExecutor, SystemListener listener, Account account, String path, BlockchainDb query) {
            return SystemImpl.create(listenerExecutor, listener, account, path, query);
        }
    };

    private CryptoApiImpl() {

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
