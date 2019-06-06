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

    default void start() {
        for (WalletManager manager: getWalletManagers()) {
            manager.connect();
        }
    }

    default void stop() {
        for (WalletManager manager: getWalletManagers()) {
            manager.disconnect();
        }
    }

    default void sync() {
        for (WalletManager manager: getWalletManagers()) {
            manager.sync();
        }
    }

    void createWalletManager(Network network, WalletManagerMode mode);

    Optional<SystemListener> getSystemListener();

    Account getAccount();

    String getPath();

    List<Network> getNetworks();

    List<WalletManager> getWalletManagers();

    default List<Wallet> getWallets() {
        List<Wallet> wallets = new ArrayList<>();
        for (WalletManager manager: getWalletManagers()) {
            wallets.addAll(manager.getWallets());
        }
        return wallets;
    }
}
