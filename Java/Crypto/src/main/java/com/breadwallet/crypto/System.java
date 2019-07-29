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

    void configure();

    void createWalletManager(Network network, WalletManagerMode mode, AddressScheme addressScheme);

    void stop();

    void subscribe(String subscriptionToken);

    Account getAccount();

    String getPath();

    List<? extends Network> getNetworks();

    List<? extends WalletManager> getWalletManagers();

    List<? extends Wallet> getWallets();

    AddressScheme getDefaultAddressScheme(Network network);

    List<AddressScheme> getSupportedAddressSchemes(Network network);

    boolean supportsAddressScheme(Network network, AddressScheme addressScheme);

    WalletManagerMode getDefaultWalletManagerMode(Network network);

    List<WalletManagerMode> getSupportedWalletManagerModes(Network network);

    boolean supportsWalletManagerModes(Network network, WalletManagerMode mode);

    void addSystemListener(SystemListener listener);

    void removeSystemListener(SystemListener listener);

    void addWalletManagerListener(WalletManager manager, SystemListener listener);

    void removeWalletManagerListener(WalletManager manager, SystemListener listener);

    void addWalletListener(Wallet wallet, SystemListener listener);

    void removeWalletListener(Wallet wallet, SystemListener listener);

    void addTransferListener(Transfer transfer, SystemListener listener);

    void removeTransferListener(Transfer transfer, SystemListener listener);
}
