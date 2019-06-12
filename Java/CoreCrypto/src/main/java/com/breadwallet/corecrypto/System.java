/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.events.network.NetworkCreatedEvent;
import com.breadwallet.crypto.events.system.SystemCreatedEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.system.SystemManagerAddedEvent;
import com.breadwallet.crypto.events.system.SystemNetworkAddedEvent;
import com.google.common.base.Optional;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/* package */
final class System implements com.breadwallet.crypto.System {

    public static System create(ExecutorService listenerExecutor, SystemListener listener, com.breadwallet.crypto.Account account, String path, BlockchainDb query) {
        return new System(listenerExecutor, listener, account, path, query);
    }

    private final SystemAnnouncer announcer;
    private final Account account;
    private final String path;
    private final BlockchainDb query;

    private final Lock networksReadLock;
    private final Lock networksWriteLock;
    private final List<Network> networks;
    private final Lock walletManagersReadLock;
    private final Lock walletManagersWriteLock;
    private final List<WalletManager> walletManagers;

    private System(ExecutorService listenerExecutor, SystemListener listener, com.breadwallet.crypto.Account account, String path, BlockchainDb query) {
        this.announcer = new SystemAnnouncer(this, listenerExecutor, listener);
        this.account = Account.from(account);
        this.path = path;
        this.query = query;

        ReadWriteLock networksRwLock = new ReentrantReadWriteLock();
        this.networksReadLock = networksRwLock.readLock();
        this.networksWriteLock = networksRwLock.writeLock();
        this.networks = new ArrayList<>();

        ReadWriteLock walletManagersRwLock = new ReentrantReadWriteLock();
        this.walletManagersReadLock = walletManagersRwLock.readLock();
        this.walletManagersWriteLock = walletManagersRwLock.writeLock();
        this.walletManagers = new ArrayList<>();

        announcer.announceSystemEvent(new SystemCreatedEvent());
    }

    @Override
    public void subscribe(String subscriptionToken) {
        // TODO(fix): Implement this!
    }

    @Override
    public void initialize(List<String> networksNeeded, boolean isMainnet) {
        NetworkDiscovery.discoverNetworks(query, networksNeeded, isMainnet, discoveredNetworks -> {
            for (Network network: discoveredNetworks) {
                if (addNetwork(network)) {
                    announcer.announceNetworkEvent(network, new NetworkCreatedEvent());
                    announcer.announceSystemEvent(new SystemNetworkAddedEvent(network));
                }
            }
        });
    }

    @Override
    public void start() {
        for (WalletManager manager: getWalletManagers()) {
            manager.connect();
        }
    }

    @Override
    public void stop() {
        for (WalletManager manager: getWalletManagers()) {
            manager.disconnect();
        }
    }

    @Override
    public void sync() {
        for (WalletManager manager: getWalletManagers()) {
            manager.sync();
        }
    }

    @Override
    public void createWalletManager(com.breadwallet.crypto.Network network, WalletManagerMode mode) {
        Network networkImpl = Network.from(network);
        String networkCode = networkImpl.getCurrency().getCode();
        if (networkCode.equals(com.breadwallet.crypto.Currency.CODE_AS_BTC)) {
            WalletManager walletManager = WalletManager.create(account, networkImpl, mode, path, announcer, query);
            if (addWalletManager(walletManager)) {
                announcer.announceSystemEvent(new SystemManagerAddedEvent(walletManager));
            }
        } else {
            throw new IllegalArgumentException("Unsupported network type");
        }
    }

    @Override
    public Optional<SystemListener> getSystemListener() {
        return announcer.getListener();
    }

    @Override
    public Account getAccount() {
        return account;
    }

    @Override
    public String getPath() {
        return path;
    }

    // WalletManager management

    @Override
    public List<WalletManager> getWalletManagers() {
        walletManagersReadLock.lock();
        try {
            return new ArrayList<>(walletManagers);
        } finally {
            walletManagersReadLock.unlock();
        }
    }

    @Override
    public List<Wallet> getWallets() {
        List<Wallet> wallets = new ArrayList<>();
        for (WalletManager manager: getWalletManagers()) {
            wallets.addAll(manager.getWallets());
        }
        return wallets;
    }

    private boolean addWalletManager(WalletManager walletManager) {
        boolean added;

        walletManagersWriteLock.lock();
        try {
            added = !walletManagers.contains(walletManager);
            if (added) {
                walletManagers.add(walletManager);
            }
        } finally {
            walletManagersWriteLock.unlock();
        }

        return added;
    }

    // Network management

    @Override
    public List<Network> getNetworks() {
        networksReadLock.lock();
        try {
            return new ArrayList<>(networks);
        } finally {
            networksReadLock.unlock();
        }
    }

    private boolean addNetwork(Network network) {
        boolean added;

        networksWriteLock.lock();
        try {
            added = !networks.contains(network);
            if (added) {
                networks.add(network);
            }
        } finally {
            networksWriteLock.unlock();
        }

        return added;
    }
}
