/*
 * System
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.events.system.SystemListener;
import com.google.common.base.Optional;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;

public interface System {

    static System create(ExecutorService listenerExecutor, SystemListener listener, Account account, String path, BlockchainDb query) {
        return CryptoApi.getProvider().systemProvider().create(listenerExecutor, listener, account, path, query);
    }

    void subscribe(String subscriptionToken);

    default void initialize(List<String> networksNeeded) {
        initialize(networksNeeded, true);
    }

    void initialize(List<String> networksNeeded, boolean isMainnet);

    void start();

    void stop();

    void sync();

    void createWalletManager(Network network, WalletManagerMode mode);

    Optional<SystemListener> getSystemListener();

    Account getAccount();

    String getPath();

    List<? extends Network> getNetworks();

    List<? extends WalletManager> getWalletManagers();

    List<? extends Wallet> getWallets();
}
