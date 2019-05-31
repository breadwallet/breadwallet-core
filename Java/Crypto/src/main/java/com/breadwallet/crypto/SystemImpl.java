package com.breadwallet.crypto;

import android.util.Log;

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

import static com.google.common.base.Preconditions.checkState;

// TODO: Revisit thread model

public final class SystemImpl implements System {

    private static final String TAG = SystemImpl.class.getName();

    public static System create(ExecutorService executorService, SystemListener listener, Account account, String path, BlockchainDb query) {
        checkState(INSTANCE == null);
        INSTANCE = new SystemImpl(executorService, listener, account, path, query);
        return INSTANCE;
    }

    private static volatile SystemImpl INSTANCE;

    private final SystemAnnouncer announcer;
    private final Account account;
    private final String path;
    private final BlockchainDb query;

    private final Lock networksReadLock;
    private final Lock networksWriteLock;
    private final List<Network> networks;
    private final Lock walletManagersReadLock;
    private final Lock walletManagersWriteLock;
    private final List<WalletManagerImpl> walletManagers;

    private SystemImpl(ExecutorService listenerExecutor, SystemListener listener, Account account, String path, BlockchainDb query) {
        checkState(INSTANCE == null);

        this.announcer = new SystemAnnouncer(this, listenerExecutor, listener);
        this.account = account;
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

    // Public Interface

    @Override
    public void subscribe(String subscriptionToken) {
        // TODO: Implement this!
    }

    @Override
    public void initialize(List<String> networksNeeded) {
        NetworkDiscovery.discoverNetworks(query, networksNeeded, discoveredNetworks -> {
            for (Network network: discoveredNetworks) {
                if (addNetwork(network)) {
                    announcer.announceNetworkEvent(network, new NetworkCreatedEvent());
                    announcer.announceSystemEvent(new SystemNetworkAddedEvent(network));
                }
            }
        });
    }

    @Override
    public void createWalletManager(Network network, WalletManagerMode mode) {
        String networkCode = network.getCurrency().getCode();
        if (networkCode.equals(com.breadwallet.crypto.Currency.CODE_AS_BTC)) {
            WalletManagerImpl walletManager = new WalletManagerImplBtc(account, network, mode, path, announcer, query);
            if (addWalletManager(walletManager)) {
                walletManager.initialize();
                announcer.announceSystemEvent(new SystemManagerAddedEvent(walletManager));
            }
        } else {
            // TODO: How do we want to handle this?
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
            // TODO: This is from the swift code but I don't see how it would ever evaluate to true
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
            // TODO: The swift code has no guards against a network being added multiple times; should it?
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
