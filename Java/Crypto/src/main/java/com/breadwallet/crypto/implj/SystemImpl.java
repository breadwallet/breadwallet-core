/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Account;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.System;
import com.breadwallet.crypto.WalletManager;
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
final class SystemImpl implements System {

    public static System create(ExecutorService listenerExecutor, SystemListener listener, Account account, String path, BlockchainDb query) {
        return new SystemImpl(listenerExecutor, listener, account, path, query);
    }

    private final SystemAnnouncer announcer;
    private final AccountImpl account;
    private final String path;
    private final BlockchainDb query;

    private final Lock networksReadLock;
    private final Lock networksWriteLock;
    private final List<NetworkImpl> networks;
    private final Lock walletManagersReadLock;
    private final Lock walletManagersWriteLock;
    private final List<WalletManagerImpl> walletManagers;

    private SystemImpl(ExecutorService listenerExecutor, SystemListener listener, Account account, String path, BlockchainDb query) {
        this.announcer = new SystemAnnouncer(this, listenerExecutor, listener);
        this.account = AccountImpl.from(account);
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
            for (NetworkImpl network: discoveredNetworks) {
                if (addNetwork(network)) {
                    announcer.announceNetworkEvent(network, new NetworkCreatedEvent());
                    announcer.announceSystemEvent(new SystemNetworkAddedEvent(network));
                }
            }
        });
    }

    @Override
    public void createWalletManager(Network network, WalletManagerMode mode) {
        NetworkImpl networkImpl = NetworkImpl.from(network);
        String networkCode = networkImpl.getCurrency().getCode();
        if (networkCode.equals(com.breadwallet.crypto.Currency.CODE_AS_BTC)) {
            WalletManagerImpl walletManager = new WalletManagerImplBtc(account, networkImpl, mode, path, announcer, query);
            if (addWalletManager(walletManager)) {
                walletManager.initialize();
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

    private boolean addWalletManager(WalletManagerImpl walletManager) {
        boolean added;

        walletManagersWriteLock.lock();
        try {
            // TODO(discuss): This is from the swift code but I don't see how it would ever evaluate to true
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

    private boolean addNetwork(NetworkImpl network) {
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
