/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;
import android.util.Log;

import com.breadwallet.corenative.crypto.BRCryptoCWMClient;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientBtc;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientCallbackState;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientEth;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientGen;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener.BRCryptoCWMListenerWalletManagerEvent;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener.BRCryptoCWMListenerWalletEvent;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener.BRCryptoCWMListenerTransferEvent;
import com.breadwallet.corenative.crypto.BRCryptoTransfer;
import com.breadwallet.corenative.crypto.BRCryptoTransferEvent;
import com.breadwallet.corenative.crypto.BRCryptoTransferEventType;
import com.breadwallet.corenative.crypto.BRCryptoWallet;
import com.breadwallet.corenative.crypto.BRCryptoWalletEvent;
import com.breadwallet.corenative.crypto.BRCryptoWalletEventType;
import com.breadwallet.corenative.crypto.BRCryptoWalletManager;
import com.breadwallet.corenative.crypto.BRCryptoWalletManagerEvent;
import com.breadwallet.corenative.crypto.BRCryptoWalletManagerEventType;
import com.breadwallet.corenative.crypto.CoreBRCryptoAmount;
import com.breadwallet.corenative.crypto.CoreBRCryptoFeeBasis;
import com.breadwallet.corenative.crypto.CoreBRCryptoTransfer;
import com.breadwallet.corenative.crypto.CoreBRCryptoWallet;
import com.breadwallet.corenative.crypto.CoreBRCryptoWalletManager;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.TransferState;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletManagerState;
import com.breadwallet.crypto.WalletState;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.breadwallet.crypto.blockchaindb.models.brd.EthToken;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;
import com.breadwallet.crypto.events.network.NetworkCreatedEvent;
import com.breadwallet.crypto.events.system.SystemCreatedEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.system.SystemManagerAddedEvent;
import com.breadwallet.crypto.events.system.SystemNetworkAddedEvent;
import com.breadwallet.crypto.events.transfer.TransferChangedEvent;
import com.breadwallet.crypto.events.transfer.TransferCreatedEvent;
import com.breadwallet.crypto.events.transfer.TransferDeletedEvent;
import com.breadwallet.crypto.events.transfer.TransferListener;
import com.breadwallet.crypto.events.wallet.WalletBalanceUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletFeeBasisUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletListener;
import com.breadwallet.crypto.events.wallet.WalletTransferAddedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferSubmittedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerBlockUpdatedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerChangedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerCreatedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerDeletedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerListener;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncProgressEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStartedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStoppedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletAddedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletChangedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletDeletedEvent;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/* package */
final class System implements com.breadwallet.crypto.System {

    private static final String TAG = System.class.getName();

    private static final Map<Pointer, WeakReference<System>> SYSTEMS = new ConcurrentHashMap<>();
    private static final AtomicInteger SYSTEM_IDS = new AtomicInteger(0);

    //
    // Keep a static reference to the callbacks so that they are never GC'ed
    //

    private static final BRCryptoCWMClientBtc CWM_CLIENT_BTC = new BRCryptoCWMClientBtc(
            System::btcGetBlockNumber,
            System::btcGetTransactions,
            System::btcSubmitTransaction
    );

    private static final BRCryptoCWMClientEth CWM_CLIENT_ETH = new BRCryptoCWMClientEth(
            System::ethGetEtherBalance,
            System::ethGetTokenBalance,
            System::ethGetGasPrice,
            System::ethEstimateGas,
            System::ethSubmitTransaction,
            System::ethGetTransactions,
            System::ethGetLogs,
            System::ethGetBlocks,
            System::ethGetTokens,
            System::ethGetBlockNumber,
            System::ethGetNonce
    );

    private static final BRCryptoCWMClientGen CWM_CLIENT_GEN = new BRCryptoCWMClientGen(
            System::genGetBlockNumber,
            System::genGetTransactions,
            System::genSubmitTransaction
    );

    private static final BRCryptoCWMListenerWalletManagerEvent CWM_LISTENER_WALLET_MANAGER_CALLBACK = System::walletManagerEventCallback;
    private static final BRCryptoCWMListenerWalletEvent CWM_LISTENER_WALLET_CALLBACK = System::walletEventCallback;
    private static final BRCryptoCWMListenerTransferEvent CWM_LISTENER_TRANSFER_CALLBACK = System::transferEventCallback;

    /* package */
    static System create(ScheduledExecutorService asyncExecutor, SystemListener listener, com.breadwallet.crypto.Account account, String path, BlockchainDb query) {
        Pointer context = Pointer.createConstant(SYSTEM_IDS.incrementAndGet());

        BRCryptoCWMListener.ByValue cwmListener = new BRCryptoCWMListener.ByValue(context,
                CWM_LISTENER_WALLET_MANAGER_CALLBACK,
                CWM_LISTENER_WALLET_CALLBACK,
                CWM_LISTENER_TRANSFER_CALLBACK);

        BRCryptoCWMClient.ByValue cwmClient = new BRCryptoCWMClient.ByValue(context,
                CWM_CLIENT_BTC,
                CWM_CLIENT_ETH,
                CWM_CLIENT_GEN);

        System system = new System(asyncExecutor,
                listener,
                account,
                path,
                query,
                cwmListener,
                cwmClient);

        SYSTEMS.put(context,
                new WeakReference<>(system));

        return system;
    }

    private static Optional<System> getSystem(Pointer context) {
        WeakReference<System> ref = SYSTEMS.get(context);
        return null != ref ? Optional.fromNullable(ref.get()): Optional.absent();
    }

    private final SystemCallbackCoordinator callbackCoordinator;
    private final SystemEventCoordinator eventCoordinator;
    private final Account account;
    private final String path;
    private final BlockchainDb query;
    private final BRCryptoCWMListener.ByValue cwmListener;
    private final BRCryptoCWMClient.ByValue cwmClient;

    private final Lock networksReadLock;
    private final Lock networksWriteLock;
    private final List<Network> networks;
    private final Lock walletManagersReadLock;
    private final Lock walletManagersWriteLock;
    private final List<WalletManager> walletManagers;

    private System(ScheduledExecutorService executor, SystemListener listener, com.breadwallet.crypto.Account account, String path,
                   BlockchainDb query, BRCryptoCWMListener.ByValue cwmListener, BRCryptoCWMClient.ByValue cwmClient) {
        this.callbackCoordinator = new SystemCallbackCoordinator(executor);
        this.eventCoordinator = new SystemEventCoordinator(executor, this, listener);
        this.account = Account.from(account);
        this.path = path;
        this.query = query;
        this.cwmListener = cwmListener;
        this.cwmClient = cwmClient;

        ReadWriteLock networksRwLock = new ReentrantReadWriteLock();
        this.networksReadLock = networksRwLock.readLock();
        this.networksWriteLock = networksRwLock.writeLock();
        this.networks = new ArrayList<>();

        ReadWriteLock walletManagersRwLock = new ReentrantReadWriteLock();
        this.walletManagersReadLock = walletManagersRwLock.readLock();
        this.walletManagersWriteLock = walletManagersRwLock.writeLock();
        this.walletManagers = new ArrayList<>();

        eventCoordinator.announceSystemEvent(new SystemCreatedEvent());
    }

    @Override
    public void configure() {
        NetworkDiscovery.discoverNetworks(query, discoveredNetworks -> {
            for (Network network: discoveredNetworks) {
                if (addNetwork(network)) {
                    eventCoordinator.announceNetworkEvent(network, new NetworkCreatedEvent());
                    eventCoordinator.announceSystemEvent(new SystemNetworkAddedEvent(network));
                }
            }
        });
    }

    @Override
    public void createWalletManager(com.breadwallet.crypto.Network network, WalletManagerMode mode, AddressScheme scheme) {
        WalletManager walletManager = WalletManager.create(cwmListener, cwmClient, account, Network.from(network), mode, scheme, path, this, callbackCoordinator);
        addWalletManager(walletManager);
        eventCoordinator.announceSystemEvent(new SystemManagerAddedEvent(walletManager));
    }

    @Override
    public void stop() {
        for (WalletManager manager: getWalletManagers()) {
            manager.disconnect();
        }
    }

    @Override
    public void subscribe(String subscriptionToken) {
        // TODO(fix): Implement this!
    }

    @Override
    public Account getAccount() {
        return account;
    }

    @Override
    public String getPath() {
        return path;
    }

    @Override
    public List<Wallet> getWallets() {
        List<Wallet> wallets = new ArrayList<>();
        for (WalletManager manager: getWalletManagers()) {
            wallets.addAll(manager.getWallets());
        }
        return wallets;
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

    private void addWalletManager(WalletManager walletManager) {
        walletManagersWriteLock.lock();
        try {
            if (!walletManagers.contains(walletManager)) {
                walletManagers.add(walletManager);
            }
        } finally {
            walletManagersWriteLock.unlock();
        }
    }

    private Optional<WalletManager> getWalletManager(CoreBRCryptoWalletManager coreWalletManager) {
        WalletManager walletManager = WalletManager.create(coreWalletManager, this, callbackCoordinator);
        walletManagersReadLock.lock();
        try {
            int index = walletManagers.indexOf(walletManager);
            return Optional.fromNullable(index == -1 ? null : walletManagers.get(index));
        } finally {
            walletManagersReadLock.unlock();
        }
    }

    private WalletManager getWalletManagerOrCreate(CoreBRCryptoWalletManager coreWalletManager) {
        WalletManager walletManager = WalletManager.create(coreWalletManager, this, callbackCoordinator);
        walletManagersWriteLock.lock();
        try {
            int index = walletManagers.indexOf(walletManager);
            if (index == -1) {
                walletManagers.add(walletManager);
            } else {
                walletManager = walletManagers.get(index);
            }
            return walletManager;
        } finally {
            walletManagersWriteLock.unlock();
        }
    }

    // Miscellaneous

    @Override
    public AddressScheme getDefaultAddressScheme(com.breadwallet.crypto.Network network) {
        switch (network.getCurrency().getCode()) {
            case com.breadwallet.crypto.Currency.CODE_AS_BTC:
                return AddressScheme.BTC_SEGWIT;

            case com.breadwallet.crypto.Currency.CODE_AS_BCH:
                return AddressScheme.BTC_LEGACY;

            case com.breadwallet.crypto.Currency.CODE_AS_ETH:
                return AddressScheme.ETH_DEFAULT;

            default:
                return AddressScheme.GEN_DEFAULT;
        }
    }

    @Override
    public List<AddressScheme> getSupportedAddressSchemes(com.breadwallet.crypto.Network network) {
        switch (network.getCurrency().getCode()) {
            case com.breadwallet.crypto.Currency.CODE_AS_BTC:
                return Arrays.asList(AddressScheme.BTC_SEGWIT, AddressScheme.BTC_LEGACY);

            case com.breadwallet.crypto.Currency.CODE_AS_BCH:
                return Collections.singletonList(AddressScheme.BTC_LEGACY);

            case com.breadwallet.crypto.Currency.CODE_AS_ETH:
                return Collections.singletonList(AddressScheme.ETH_DEFAULT);

            default:
                return Collections.singletonList(AddressScheme.GEN_DEFAULT);
        }
    }

    @Override
    public boolean supportsAddressScheme(com.breadwallet.crypto.Network network, AddressScheme addressScheme) {
        return getSupportedAddressSchemes(network).contains(addressScheme);
    }

    @Override
    public WalletManagerMode getDefaultWalletManagerMode(com.breadwallet.crypto.Network network) {
        switch (network.getCurrency().getCode()) {
            case com.breadwallet.crypto.Currency.CODE_AS_BTC:
                return WalletManagerMode.P2P_ONLY;

            case com.breadwallet.crypto.Currency.CODE_AS_BCH:
                return WalletManagerMode.P2P_ONLY;

            case com.breadwallet.crypto.Currency.CODE_AS_ETH:
                return WalletManagerMode.API_ONLY;

            default:
                return WalletManagerMode.API_ONLY;
        }
    }

    @Override
    public List<WalletManagerMode> getSupportedWalletManagerModes(com.breadwallet.crypto.Network network) {
        switch (network.getCurrency().getCode()) {
            case com.breadwallet.crypto.Currency.CODE_AS_BTC:
                return Arrays.asList(WalletManagerMode.API_ONLY, WalletManagerMode.P2P_ONLY);

            case com.breadwallet.crypto.Currency.CODE_AS_BCH:
                return Collections.singletonList(WalletManagerMode.P2P_ONLY);

            case com.breadwallet.crypto.Currency.CODE_AS_ETH:
                return Arrays.asList(WalletManagerMode.API_ONLY, WalletManagerMode.API_WITH_P2P_SUBMIT);

            default:
                return Collections.singletonList(WalletManagerMode.API_ONLY);
        }
    }

    @Override
    public boolean supportsWalletManagerModes(com.breadwallet.crypto.Network network, WalletManagerMode mode) {
        return getSupportedWalletManagerModes(network).contains(mode);
    }

    @Override
    public void addSystemListener(SystemListener listener) {
        eventCoordinator.addSystemListener(listener);
    }

    @Override
    public void removeSystemListener(SystemListener listener) {
        eventCoordinator.removeSystemListener(listener);
    }

    @Override
    public void addWalletManagerListener(com.breadwallet.crypto.WalletManager manager, WalletManagerListener listener) {
        eventCoordinator.addWalletManagerListener(manager, listener);
    }

    @Override
    public void removeWalletManagerListener(com.breadwallet.crypto.WalletManager manager, WalletManagerListener listener) {
        eventCoordinator.removeWalletManagerListener(manager, listener);
    }

    @Override
    public void addWalletListener(com.breadwallet.crypto.Wallet wallet, WalletListener listener) {
        eventCoordinator.addWalletListener(wallet, listener);
    }

    @Override
    public void removeWalletListener(com.breadwallet.crypto.Wallet wallet, WalletListener listener) {
        eventCoordinator.removeWalletListener(wallet, listener);
    }

    @Override
    public void addTransferListener(com.breadwallet.crypto.Transfer transfer, TransferListener listener) {
        eventCoordinator.addTransferListener(transfer, listener);
    }

    @Override
    public void removeTransferListener(com.breadwallet.crypto.Transfer transfer, TransferListener listener) {
        eventCoordinator.removeTransferListener(transfer, listener);
    }

    //
    // WalletManager Events
    //

    private static void walletManagerEventCallback(Pointer context, @Nullable BRCryptoWalletManager coreWalletManager,
                                            BRCryptoWalletManagerEvent.ByValue event) {
        Log.d(TAG, "WalletManagerEventCallback");

        CoreBRCryptoWalletManager walletManager = CoreBRCryptoWalletManager.createOwned(coreWalletManager);

        switch (event.type) {
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_CREATED: {
                handleWalletManagerCreated(context, walletManager, event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_CHANGED: {
                handleWalletManagerChanged(context, walletManager, event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_DELETED: {
                handleWalletManagerDeleted(context, walletManager, event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED: {
                handleWalletManagerWalletAdded(context, walletManager, event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED: {
                handleWalletManagerWalletChanged(context, walletManager, event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED: {
                handleWalletManagerWalletDeleted(context, walletManager, event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED: {
                handleWalletManagerSyncStarted(context, walletManager, event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES: {
                handleWalletManagerSyncProgress(context, walletManager, event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED: {
                handleWalletManagerSyncStopped(context, walletManager, event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED: {
                handleWalletManagerBlockHeightUpdated(context, walletManager, event);
                break;
            }
        }
    }

    private static void handleWalletManagerCreated(Pointer context, CoreBRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerCreated");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            WalletManager walletManager = system.getWalletManagerOrCreate(coreWalletManager);
            system.eventCoordinator.announceWalletManagerEvent(walletManager, new WalletManagerCreatedEvent());

        } else {
            Log.e(TAG, "WalletManagerCreated: missed system");
        }
    }

    private static void handleWalletManagerChanged(Pointer context, CoreBRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        WalletManagerState oldState = Utilities.walletManagerStateFromCrypto(event.u.state.oldValue);
        WalletManagerState newState = Utilities.walletManagerStateFromCrypto(event.u.state.newValue);

        Log.d(TAG, String.format("WalletManagerChanged (%s -> %s)", oldState, newState));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.eventCoordinator.announceWalletManagerEvent(walletManager, new WalletManagerChangedEvent(oldState, newState));

            } else {
                Log.e(TAG, "WalletManagerChanged: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletManagerChanged: missed system");
        }
    }

    private static void handleWalletManagerDeleted(Pointer context, CoreBRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerDeleted");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.eventCoordinator.announceWalletManagerEvent(walletManager, new WalletManagerDeletedEvent());

            } else {
                Log.e(TAG, "WalletManagerDeleted: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletManagerDeleted: missed system");
        }
    }

    private static void handleWalletManagerWalletAdded(Pointer context, CoreBRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerWalletAdded");

        CoreBRCryptoWallet coreWallet = CoreBRCryptoWallet.createOwned(event.u.wallet.value);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optional = walletManager.getWallet(coreWallet);
                if (optional.isPresent()) {
                    Wallet wallet = optional.get();
                    system.eventCoordinator.announceWalletManagerEvent(walletManager, new WalletManagerWalletAddedEvent(wallet));

                } else {
                    Log.e(TAG, "WalletManagerWalletAdded: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletManagerWalletAdded: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletManagerWalletAdded: missed system");
        }
    }

    private static void handleWalletManagerWalletChanged(Pointer context, CoreBRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerWalletChanged");

        CoreBRCryptoWallet coreWallet = CoreBRCryptoWallet.createOwned(event.u.wallet.value);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optional = walletManager.getWallet(coreWallet);
                if (optional.isPresent()) {
                    Wallet wallet = optional.get();
                    system.eventCoordinator.announceWalletManagerEvent(walletManager, new WalletManagerWalletChangedEvent(wallet));

                } else {
                    Log.e(TAG, "WalletManagerWalletChanged: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletManagerWalletChanged: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletManagerWalletChanged: missed system");
        }
    }

    private static void handleWalletManagerWalletDeleted(Pointer context, CoreBRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerWalletDeleted");

        CoreBRCryptoWallet coreWallet = CoreBRCryptoWallet.createOwned(event.u.wallet.value);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optional = walletManager.getWallet(coreWallet);
                if (optional.isPresent()) {
                    Wallet wallet = optional.get();
                    system.eventCoordinator.announceWalletManagerEvent(walletManager, new WalletManagerWalletDeletedEvent(wallet));

                } else {
                    Log.e(TAG, "WalletManagerWalletDeleted: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletManagerWalletDeleted: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletManagerWalletDeleted: missed system");
        }
    }

    private static void handleWalletManagerSyncStarted(Pointer context, CoreBRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerSyncStarted");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.eventCoordinator.announceWalletManagerEvent(walletManager, new WalletManagerSyncStartedEvent());

            } else {
                Log.e(TAG, "WalletManagerSyncStarted: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletManagerSyncStarted: missed system");
        }
    }

    private static void handleWalletManagerSyncProgress(Pointer context, CoreBRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        double percent = event.u.sync.percentComplete;

        Log.d(TAG, String.format("WalletManagerSyncProgress (%s)", percent));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.eventCoordinator.announceWalletManagerEvent(walletManager, new WalletManagerSyncProgressEvent(percent));

            } else {
                Log.e(TAG, "WalletManagerSyncProgress: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletManagerSyncProgress: missed system");
        }
    }

    private static void handleWalletManagerSyncStopped(Pointer context, CoreBRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerSyncStopped");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                // TODO(fix): fill in message
                system.eventCoordinator.announceWalletManagerEvent(walletManager, new WalletManagerSyncStoppedEvent(""));

            } else {
                Log.e(TAG, "WalletManagerSyncStopped: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletManagerSyncStopped: missed system");
        }
    }

    private static void handleWalletManagerBlockHeightUpdated(Pointer context, CoreBRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        UnsignedLong blockHeight = UnsignedLong.fromLongBits(event.u.blockHeight.value);

        Log.d(TAG, String.format("WalletManagerBlockHeightUpdated (%s)", blockHeight));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.eventCoordinator.announceWalletManagerEvent(walletManager, new WalletManagerBlockUpdatedEvent(blockHeight));

            } else {
                Log.e(TAG, "WalletManagerBlockHeightUpdated: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletManagerBlockHeightUpdated: missed system");
        }
    }

    //
    // Wallet Events
    //

    private static void walletEventCallback(Pointer context, @Nullable BRCryptoWalletManager coreWalletManager,
                                     @Nullable BRCryptoWallet coreWallet, BRCryptoWalletEvent.ByValue event) {
        Log.d(TAG, "WalletEventCallback");

        CoreBRCryptoWalletManager walletManager = CoreBRCryptoWalletManager.createOwned(coreWalletManager);
        CoreBRCryptoWallet wallet = CoreBRCryptoWallet.createOwned(coreWallet);

        switch (event.type) {
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_CREATED: {
                handleWalletCreated(context, walletManager, wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_CHANGED: {
                handleWalletChanged(context, walletManager, wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_DELETED: {
                handleWalletDeleted(context, walletManager, wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_ADDED: {
                handleWalletTransferAdded(context, walletManager, wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_CHANGED: {
                handleWalletTransferChanged(context, walletManager, wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED: {
                handleWalletTransferSubmitted(context, walletManager, wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_DELETED: {
                handleWalletTransferDeleted(context, walletManager, wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_BALANCE_UPDATED: {
                handleWalletBalanceUpdated(context, walletManager, wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED: {
                handleWalletFeeBasisUpdated(context, walletManager, wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED: {
                handleWalletFeeBasisEstimated(context, walletManager, wallet, event);
                break;
            }
        }
    }

    private static void handleWalletCreated(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletCreated");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWalletOrCreate(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    system.eventCoordinator.announceWalletEvent(walletManager, wallet, new WalletCreatedEvent());

                } else {
                    Log.e(TAG, "WalletCreated: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletCreated: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletCreated: missed system");
        }
    }

    private static void handleWalletChanged(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        WalletState oldState = Utilities.walletStateFromCrypto(event.u.state.oldState);
        WalletState newState = Utilities.walletStateFromCrypto(event.u.state.newState);

        Log.d(TAG, String.format("WalletChanged (%s -> %s)", oldState, newState));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    system.eventCoordinator.announceWalletEvent(walletManager, wallet, new WalletChangedEvent(oldState, newState));

                } else {
                    Log.e(TAG, "WalletChanged: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletChanged: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletChanged: missed system");
        }
    }

    private static void handleWalletDeleted(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletDeleted");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    system.eventCoordinator.announceWalletEvent(walletManager, wallet, new WalletDeletedEvent());

                } else {
                    Log.e(TAG, "WalletDeleted: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletDeleted: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletDeleted: missed system");
        }
    }

    private static void handleWalletTransferAdded(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletTransferAdded");

        CoreBRCryptoTransfer coreTransfer = CoreBRCryptoTransfer.createOwned(event.u.transfer.value);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    Optional<Transfer> optional = wallet.getTransfer(coreTransfer);

                    if (optional.isPresent()) {
                        Transfer transfer = optional.get();
                        system.eventCoordinator.announceWalletEvent(walletManager, wallet, new WalletTransferAddedEvent(transfer));

                    } else {
                        Log.e(TAG, "WalletTransferAdded: missed transfer");
                    }

                } else {
                    Log.e(TAG, "WalletTransferAdded: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletTransferAdded: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletTransferAdded: missed system");
        }
    }

    private static void handleWalletTransferChanged(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletTransferChanged");

        CoreBRCryptoTransfer coreTransfer = CoreBRCryptoTransfer.createOwned(event.u.transfer.value);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    Optional<Transfer> optional = wallet.getTransfer(coreTransfer);

                    if (optional.isPresent()) {
                        Transfer transfer = optional.get();
                        system.eventCoordinator.announceWalletEvent(walletManager, wallet, new WalletTransferChangedEvent(transfer));

                    } else {
                        Log.e(TAG, "WalletTransferChanged: missed transfer");
                    }

                } else {
                    Log.e(TAG, "WalletTransferChanged: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletTransferChanged: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletTransferChanged: missed system");
        }
    }

    private static void handleWalletTransferSubmitted(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletTransferSubmitted");

        CoreBRCryptoTransfer coreTransfer = CoreBRCryptoTransfer.createOwned(event.u.transfer.value);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    Optional<Transfer> optional = wallet.getTransfer(coreTransfer);

                    if (optional.isPresent()) {
                        Transfer transfer = optional.get();
                        system.eventCoordinator.announceWalletEvent(walletManager, wallet, new WalletTransferSubmittedEvent(transfer));

                    } else {
                        Log.e(TAG, "WalletTransferSubmitted: missed transfer");
                    }

                } else {
                    Log.e(TAG, "WalletTransferSubmitted: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletTransferSubmitted: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletTransferSubmitted: missed system");
        }
    }

    private static void handleWalletTransferDeleted(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletTransferDeleted");

        CoreBRCryptoTransfer coreTransfer = CoreBRCryptoTransfer.createOwned(event.u.transfer.value);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    Optional<Transfer> optional = wallet.getTransfer(coreTransfer);

                    if (optional.isPresent()) {
                        Transfer transfer = optional.get();
                        system.eventCoordinator.announceWalletEvent(walletManager, wallet, new WalletTransferDeletedEvent(transfer));

                    } else {
                        Log.e(TAG, "WalletTransferDeleted: missed transfer");
                    }

                } else {
                    Log.e(TAG, "WalletTransferDeleted: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletTransferDeleted: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletTransferDeleted: missed system");
        }
    }

    private static void handleWalletBalanceUpdated(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletBalanceUpdated");

        CoreBRCryptoAmount coreAmount = CoreBRCryptoAmount.createOwned(event.u.balanceUpdated.amount);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    Amount amount = Amount.create(coreAmount, wallet.getUnit());
                    Log.d(TAG, String.format("WalletBalanceUpdated: %s", amount));
                    system.eventCoordinator.announceWalletEvent(walletManager, wallet, new WalletBalanceUpdatedEvent(amount));

                } else {
                    Log.e(TAG, "WalletBalanceUpdated: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletBalanceUpdated: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletBalanceUpdated: missed system");
        }
    }

    private static void handleWalletFeeBasisUpdated(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletFeeBasisUpdate");

        CoreBRCryptoFeeBasis coreFeeBasis = CoreBRCryptoFeeBasis.createOwned(event.u.feeBasisUpdated.basis);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    TransferFeeBasis feeBasis = TransferFeeBasis.create(coreFeeBasis);
                    Log.d(TAG, String.format("WalletFeeBasisUpdate: %s", feeBasis));
                    system.eventCoordinator.announceWalletEvent(walletManager, wallet, new WalletFeeBasisUpdatedEvent(feeBasis));

                } else {
                    Log.e(TAG, "WalletFeeBasisUpdate: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletFeeBasisUpdate: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletFeeBasisUpdate: missed system");
        }
    }

    private static void handleWalletFeeBasisEstimated(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletFeeBasisEstimated");

        CoreBRCryptoFeeBasis coreFeeBasis = CoreBRCryptoFeeBasis.createOwned(event.u.feeBasisEstimated.basis);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    TransferFeeBasis feeBasis = TransferFeeBasis.create(coreFeeBasis);
                    Log.d(TAG, String.format("WalletFeeBasisEstimated: %s", feeBasis));
                    system.callbackCoordinator.completeFeeBasisEstimateHandlerWithSuccess(event.u.feeBasisEstimated.cookie, feeBasis);

                } else {
                    Log.e(TAG, "WalletFeeBasisEstimated: missed wallet");
                }

            } else {
                Log.e(TAG, "WalletFeeBasisEstimated: missed wallet manager");
            }

        } else {
            Log.e(TAG, "WalletFeeBasisEstimated: missed system");
        }
    }

    //
    // Transfer Events
    //

    private static void transferEventCallback(Pointer context, @Nullable BRCryptoWalletManager coreWalletManager,
                                       @Nullable BRCryptoWallet coreWallet, @Nullable BRCryptoTransfer coreTransfer,
                                       BRCryptoTransferEvent.ByValue event) {
        Log.d(TAG, "TransferEventCallback");

        CoreBRCryptoWalletManager walletManager = CoreBRCryptoWalletManager.createOwned(coreWalletManager);
        CoreBRCryptoWallet wallet = CoreBRCryptoWallet.createOwned(coreWallet);
        CoreBRCryptoTransfer transfer = CoreBRCryptoTransfer.createOwned(coreTransfer);

        switch (event.type) {
            case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_CREATED: {
                handleTransferCreated(context, walletManager, wallet, transfer, event);
                break;
            }
            case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_CHANGED: {
                handleTransferChanged(context, walletManager, wallet, transfer, event);
                break;
            }
            case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_DELETED: {
                handleTransferDeleted(context, walletManager, wallet, transfer, event);
                break;
            }
        }
    }

    private static void handleTransferCreated(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, CoreBRCryptoTransfer coreTransfer,
                                       BRCryptoTransferEvent event) {
        Log.d(TAG, "TransferCreated");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();

                    Optional<Transfer> optTransfer = wallet.getTransferOrCreate(coreTransfer);
                    if (optTransfer.isPresent()) {
                        Transfer transfer = optTransfer.get();
                        system.eventCoordinator.announceTransferEvent(walletManager, wallet, transfer, new TransferCreatedEvent());

                    } else {
                        Log.e(TAG, "TransferCreated: missed transfer");
                    }

                } else {
                    Log.e(TAG, "TransferCreated: missed wallet");
                }

            } else {
                Log.e(TAG, "TransferCreated: missed wallet manager");
            }

        } else {
            Log.e(TAG, "TransferCreated: missed system");
        }
    }

    private static void handleTransferChanged(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, CoreBRCryptoTransfer coreTransfer,
                                       BRCryptoTransferEvent event) {
        // TODO(fix): Deal with memory management for the fee
        TransferState oldState = Utilities.transferStateFromCrypto(event.u.state.oldState);
        TransferState newState = Utilities.transferStateFromCrypto(event.u.state.newState);

        Log.d(TAG, String.format("TransferChanged (%s -> %s)", oldState, newState));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();

                    Optional<Transfer> optTransfer = wallet.getTransfer(coreTransfer);
                    if (optTransfer.isPresent()) {
                        Transfer transfer = optTransfer.get();

                        system.eventCoordinator.announceTransferEvent(walletManager, wallet, transfer, new TransferChangedEvent(oldState, newState));

                    } else {
                        Log.e(TAG, "TransferChanged: missed transfer");
                    }

                } else {
                    Log.e(TAG, "TransferChanged: missed wallet");
                }

            } else {
                Log.e(TAG, "TransferChanged: missed wallet manager");
            }

        } else {
            Log.e(TAG, "TransferChanged: missed system");
        }
    }

    private static void handleTransferDeleted(Pointer context, CoreBRCryptoWalletManager coreWalletManager, CoreBRCryptoWallet coreWallet, CoreBRCryptoTransfer coreTransfer,
                                       BRCryptoTransferEvent event) {
        Log.d(TAG, "TransferDeleted");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();

                    Optional<Transfer> optTransfer = wallet.getTransfer(coreTransfer);
                    if (optTransfer.isPresent()) {
                        Transfer transfer = optTransfer.get();
                        system.eventCoordinator.announceTransferEvent(walletManager, wallet, transfer, new TransferDeletedEvent());

                    } else {
                        Log.e(TAG, "TransferDeleted: missed transfer");
                    }

                } else {
                    Log.e(TAG, "TransferDeleted: missed wallet");
                }

            } else {
                Log.e(TAG, "TransferDeleted: missed wallet manager");
            }

        } else {
            Log.e(TAG, "TransferDeleted: missed system");
        }
    }

    // BTC client

    private static void btcGetBlockNumber(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState) {
        Log.d(TAG, "BRCryptoCWMBtcGetBlockNumberCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);

        if (optSystem.isPresent()) {
            optSystem.get().query.getBlockchain(coreWalletManager.getNetwork().getUids(), new CompletionHandler<Blockchain, QueryError>() {
                @Override
                public void handleData(Blockchain blockchain) {
                    UnsignedLong blockchainHeight = blockchain.getBlockHeight();
                    Log.d(TAG, String.format("BRCryptoCWMBtcGetBlockNumberCallback: succeeded (%s)", blockchainHeight));
                    coreWalletManager.announceGetBlockNumberSuccess(callbackState, blockchainHeight);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMBtcGetBlockNumberCallback: failed", error);
                    coreWalletManager.announceGetBlockNumberFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMBtcGetBlockNumberCallback: missing system");
            coreWalletManager.announceGetBlockNumberFailure(callbackState);
        }
    }

    private static void btcGetTransactions(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                    Pointer addrs, SizeT addrCount, long begBlockNumber, long endBlockNumber) {
        UnsignedLong begBlockNumberUnsigned = UnsignedLong.fromLongBits(begBlockNumber);
        UnsignedLong endBlockNumberUnsigned = UnsignedLong.fromLongBits(endBlockNumber);

        Log.d(TAG, String.format("BRCryptoCWMBtcGetTransactionsCallback (%s -> %s)", begBlockNumberUnsigned, endBlockNumberUnsigned));

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            int addressesCount = UnsignedInts.checkedCast(addrCount.longValue());
            String[] addresses = addrs.getStringArray(0, addressesCount, "UTF-8");

            optSystem.get().query.getTransactions(coreWalletManager.getNetwork().getUids(), Arrays.asList(addresses), begBlockNumberUnsigned,
                    endBlockNumberUnsigned, true,
                    false, new CompletionHandler<List<Transaction>, QueryError>() {
                        @Override
                        public void handleData(List<Transaction> transactions) {
                            Log.d(TAG, "BRCryptoCWMBtcGetTransactionsCallback received transactions");

                            for (Transaction transaction : transactions) {
                                Optional<byte[]> optRaw = transaction.getRaw();
                                if (!optRaw.isPresent()) {
                                    Log.e(TAG, "BRCryptoCWMBtcGetTransactionsCallback completing with missing raw bytes");
                                    coreWalletManager.announceGetTransactionsComplete(callbackState, false);
                                    return;
                                }

                                UnsignedLong blockHeight = transaction.getBlockHeight().or(UnsignedLong.ZERO);
                                UnsignedLong timestamp =
                                        transaction.getTimestamp().transform(Date::getTime).transform(TimeUnit.MILLISECONDS::toSeconds).transform(UnsignedLong::valueOf).or(UnsignedLong.ZERO);
                                Log.d(TAG,
                                        "BRCryptoCWMBtcGetTransactionsCallback announcing " + transaction.getId());
                                coreWalletManager.announceGetTransactionsItemBtc(callbackState, optRaw.get(), timestamp, blockHeight);
                            }

                            Log.d(TAG, "BRCryptoCWMBtcGetTransactionsCallback: complete");
                            coreWalletManager.announceGetTransactionsComplete(callbackState, true);
                        }

                        @Override
                        public void handleError(QueryError error) {
                            Log.e(TAG, "BRCryptoCWMBtcGetTransactionsCallback received an error, completing with failure", error);
                            coreWalletManager.announceGetTransactionsComplete(callbackState, false);
                        }
                    });

        } else {
            Log.e(TAG, "BRCryptoCWMBtcGetTransactionsCallback: missing system");
            coreWalletManager.announceGetTransactionsComplete(callbackState, false);
        }
    }

    private static void btcSubmitTransaction(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                             Pointer tx, SizeT txLength, String hashAsHex) {
        Log.d(TAG, "BRCryptoCWMBtcSubmitTransactionCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            byte[] txBytes = tx.getByteArray(0, UnsignedInts.checkedCast(txLength.longValue()));

            optSystem.get().query.createTransaction(coreWalletManager.getNetwork().getUids(), hashAsHex, txBytes, new CompletionHandler<Void, QueryError>() {
                @Override
                public void handleData(Void data) {
                    Log.d(TAG, "BRCryptoCWMBtcSubmitTransactionCallback: succeeded");
                    coreWalletManager.announceSubmitTransferSuccess(callbackState);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMBtcSubmitTransactionCallback: failed", error);
                    coreWalletManager.announceSubmitTransferFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMBtcSubmitTransactionCallback: missing system");
            coreWalletManager.announceSubmitTransferFailure(callbackState);
        }
    }

    // ETH client

    private static void ethGetEtherBalance(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                    String networkName, String address) {
        Log.d(TAG, "BRCryptoCWMEthGetEtherBalanceCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getBalanceAsEth(networkName, address, new CompletionHandler<String, QueryError>() {
                @Override
                public void handleData(String balance) {
                    Log.d(TAG, "BRCryptoCWMEthGetEtherBalanceCallback: succeeded");
                    coreWalletManager.announceGetBalanceSuccess(callbackState, balance);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMEthGetEtherBalanceCallback: failed", error);
                    coreWalletManager.announceGetBalanceFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMEthGetEtherBalanceCallback: missing system");
            coreWalletManager.announceGetBalanceFailure(callbackState);
        }
    }

    private static void ethGetTokenBalance(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                    String networkName, String address, String tokenAddress) {
        Log.d(TAG, "BRCryptoCWMEthGetTokenBalanceCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getBalanceAsTok(networkName, address, tokenAddress, new CompletionHandler<String, QueryError>() {
                @Override
                public void handleData(String balance) {
                    Log.d(TAG, "BRCryptoCWMEthGetTokenBalanceCallback: succeeded");
                    coreWalletManager.announceGetBalanceSuccess(callbackState, balance);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMEthGetTokenBalanceCallback: failed", error);
                    coreWalletManager.announceGetBalanceFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMEthGetTokenBalanceCallback: missing system");
            coreWalletManager.announceGetBalanceFailure(callbackState);
        }
    }

    private static void ethGetGasPrice(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                String networkName) {
        Log.d(TAG, "BRCryptoCWMEthGetGasPriceCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getGasPriceAsEth(networkName, new CompletionHandler<String, QueryError>() {
                @Override
                public void handleData(String gasPrice) {
                    Log.d(TAG, "BRCryptoCWMEthGetGasPriceCallback: succeeded");
                    coreWalletManager.announceGetGasPriceSuccess(callbackState, gasPrice);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMEthGetGasPriceCallback: failed", error);
                    coreWalletManager.announceGetGasPriceFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMEthGetGasPriceCallback: missing sytem");
            coreWalletManager.announceGetGasPriceFailure(callbackState);
        }
    }

    private static void ethEstimateGas(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                String networkName, String from, String to, String amount, String gasPrice, String data) {
        Log.d(TAG, "BRCryptoCWMEthEstimateGasCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getGasEstimateAsEth(networkName, from, to, amount, data, new CompletionHandler<String, QueryError>() {
                @Override
                public void handleData(String gasEstimate) {
                    Log.d(TAG, "BRCryptoCWMEthEstimateGasCallback: succeeded");
                    coreWalletManager.announceGetGasEstimateSuccess(callbackState, gasEstimate, gasPrice);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMEthEstimateGasCallback: failed", error);
                    coreWalletManager.announceGetGasEstimateFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMEthEstimateGasCallback: missing sytem");
            coreWalletManager.announceGetGasEstimateFailure(callbackState);
        }
    }

    private static void ethSubmitTransaction(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                      String networkName, String transaction) {
        Log.d(TAG, "BRCryptoCWMEthSubmitTransactionCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.submitTransactionAsEth(networkName, transaction, new CompletionHandler<String, QueryError>() {
                @Override
                public void handleData(String hash) {
                    Log.d(TAG, "BRCryptoCWMEthSubmitTransactionCallback: succeeded");
                    coreWalletManager.announceSubmitTransferSuccess(callbackState, hash);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMEthSubmitTransactionCallback: failed", error);
                    coreWalletManager.announceSubmitTransferFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMEthSubmitTransactionCallback: missing sytem");
            coreWalletManager.announceSubmitTransferFailure(callbackState);
        }
    }

    private static void ethGetTransactions(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                    String networkName, String address, long begBlockNumber, long endBlockNumber) {
        Log.d(TAG, "BRCryptoCWMEthGetTransactionsCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getTransactionsAsEth(networkName, address, UnsignedLong.fromLongBits(begBlockNumber),
                    UnsignedLong.fromLongBits(endBlockNumber), new CompletionHandler<List<EthTransaction>, QueryError>() {
                        @Override
                        public void handleData(List<EthTransaction> transactions) {
                            Log.d(TAG, "BRCryptoCWMEthGetTransactionsCallback: succeeded");
                            for (EthTransaction tx : transactions) {
                                coreWalletManager.announceGetTransactionsItemEth(callbackState,
                                        tx.getHash(),
                                        tx.getSourceAddr(),
                                        tx.getTargetAddr(),
                                        tx.getContractAddr(),
                                        tx.getAmount(),
                                        tx.getGasLimit(),
                                        tx.getGasPrice(),
                                        tx.getData(),
                                        tx.getNonce(),
                                        tx.getGasUsed(),
                                        tx.getBlockNumber(),
                                        tx.getBlockHash(),
                                        tx.getBlockConfirmations(),
                                        tx.getBlockTransacionIndex(),
                                        tx.getBlockTimestamp(),
                                        tx.getIsError());
                            }
                            coreWalletManager.announceGetTransactionsComplete(callbackState, true);
                        }

                        @Override
                        public void handleError(QueryError error) {
                            Log.e(TAG, "BRCryptoCWMEthGetTransactionsCallback: failed", error);
                            coreWalletManager.announceGetTransactionsComplete(callbackState, false);
                        }
                    });

        } else {
            Log.e(TAG, "BRCryptoCWMEthGetTransactionsCallback: missing system");
            coreWalletManager.announceGetTransactionsComplete(callbackState, false);
        }
    }

    private static void ethGetLogs(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                            String networkName, String contract, String address, String event, long begBlockNumber,
                            long endBlockNumber) {
        Log.d(TAG, "BRCryptoCWMEthGetLogsCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getLogsAsEth(networkName, contract, address, event, UnsignedLong.fromLongBits(begBlockNumber),
                    UnsignedLong.fromLongBits(endBlockNumber), new CompletionHandler<List<EthLog>, QueryError>() {
                        @Override
                        public void handleData(List<EthLog> logs) {
                            Log.d(TAG, "BRCryptoCWMEthGetLogsCallback: succeeded");
                            for (EthLog log : logs) {
                                coreWalletManager.announceGetLogsItem(callbackState,
                                        log.getHash(),
                                        log.getContract(),
                                        log.getTopics(),
                                        log.getData(),
                                        log.getGasPrice(),
                                        log.getGasUsed(),
                                        log.getLogIndex(),
                                        log.getBlockNumber(),
                                        log.getBlockTransactionIndex(),
                                        log.getBlockTimestamp());
                            }
                            coreWalletManager.announceGetLogsComplete(callbackState, true);
                        }

                        @Override
                        public void handleError(QueryError error) {
                            Log.e(TAG, "BRCryptoCWMEthGetLogsCallback: failed", error);
                            coreWalletManager.announceGetLogsComplete(callbackState, false);
                        }
                    });

        } else {
            Log.e(TAG, "BRCryptoCWMEthGetLogsCallback: missing system");
            coreWalletManager.announceGetLogsComplete(callbackState, false);
        }
    }

    private static void ethGetBlocks(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                              String networkName, String address, int interests, long blockNumberStart,
                              long blockNumberStop) {
        Log.d(TAG, "BRCryptoCWMEthGetBlocksCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getBlocksAsEth(networkName, address, UnsignedInteger.fromIntBits(interests),
                    UnsignedLong.fromLongBits(blockNumberStart), UnsignedLong.fromLongBits(blockNumberStop),
                    new CompletionHandler<List<UnsignedLong>, QueryError>() {
                        @Override
                        public void handleData(List<UnsignedLong> blocks) {
                            Log.d(TAG, "BRCryptoCWMEthGetBlocksCallback: succeeded");
                            coreWalletManager.announceGetBlocksSuccess(callbackState, blocks);
                        }

                        @Override
                        public void handleError(QueryError error) {
                            Log.e(TAG, "BRCryptoCWMEthGetBlocksCallback: failed", error);
                            coreWalletManager.announceGetBlocksFailure(callbackState);
                        }
                    });

        } else {
            Log.e(TAG, "BRCryptoCWMEthGetBlocksCallback: missing system");
            coreWalletManager.announceGetBlocksFailure(callbackState);
        }
    }

    private static void ethGetTokens(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState) {
        Log.d(TAG, "BREthereumClientHandlerGetTokens");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getTokensAsEth(new CompletionHandler<List<EthToken>, QueryError>() {
                @Override
                public void handleData(List<EthToken> tokens) {
                    Log.d(TAG, "BREthereumClientHandlerGetTokens: succeeded");
                    for (EthToken token : tokens) {
                        coreWalletManager.announceGetTokensItem(callbackState,
                                token.getAddress(),
                                token.getSymbol(),
                                token.getName(),
                                token.getDescription(),
                                token.getDecimals(),
                                token.getDefaultGasLimit().orNull(),
                                token.getDefaultGasPrice().orNull());
                    }
                    coreWalletManager.announceGetTokensComplete(callbackState, true);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BREthereumClientHandlerGetTokens: failed", error);
                    coreWalletManager.announceGetTokensComplete(callbackState, false);
                }
            });

        } else {
            Log.e(TAG, "BREthereumClientHandlerGetTokens: missing system");
            coreWalletManager.announceGetTokensComplete(callbackState, false);
        }
    }

    private static void ethGetBlockNumber(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                   String networkName) {
        Log.d(TAG, "BRCryptoCWMEthGetBlockNumberCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getBlockNumberAsEth(networkName, new CompletionHandler<String, QueryError>() {
                @Override
                public void handleData(String number) {
                    Log.d(TAG, "BRCryptoCWMEthGetBlockNumberCallback: succeeded");
                    coreWalletManager.announceGetBlockNumberSuccess(callbackState, number);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMEthGetBlockNumberCallback: failed", error);
                    coreWalletManager.announceGetBlockNumberFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMEthGetBlockNumberCallback: missing system");
            coreWalletManager.announceGetBlockNumberFailure(callbackState);
        }
    }

    private static void ethGetNonce(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                             String networkName, String address) {
        Log.d(TAG, "BRCryptoCWMEthGetNonceCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getNonceAsEth(networkName, address, new CompletionHandler<String, QueryError>() {
                @Override
                public void handleData(String nonce) {
                    Log.d(TAG, "BRCryptoCWMEthGetNonceCallback: succeeded");
                    coreWalletManager.announceGetNonceSuccess(callbackState, address, nonce);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMEthGetNonceCallback: failed", error);
                    coreWalletManager.announceGetNonceFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMEthGetNonceCallback: missing system");
            coreWalletManager.announceGetNonceFailure(callbackState);
        }
    }

    // GEN client

    private static void genGetBlockNumber(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState) {
        Log.d(TAG, "BRCryptoCWMGenGetBlockNumberCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);

        if (optSystem.isPresent()) {
            optSystem.get().query.getBlockchain(coreWalletManager.getNetwork().getUids(), new CompletionHandler<Blockchain, QueryError>() {
                @Override
                public void handleData(Blockchain blockchain) {
                    UnsignedLong blockchainHeight = blockchain.getBlockHeight();
                    Log.d(TAG, String.format("BRCryptoCWMGenGetBlockNumberCallback: succeeded (%s)", blockchainHeight));
                    coreWalletManager.announceGetBlockNumberSuccess(callbackState, blockchainHeight);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMGenGetBlockNumberCallback: failed", error);
                    coreWalletManager.announceGetBlockNumberFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMGenGetBlockNumberCallback: missing system");
            coreWalletManager.announceGetBlockNumberFailure(callbackState);
        }
    }

    private static void genGetTransactions(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                           String address, long begBlockNumber, long endBlockNumber) {
        UnsignedLong begBlockNumberUnsigned = UnsignedLong.fromLongBits(begBlockNumber);
        UnsignedLong endBlockNumberUnsigned = UnsignedLong.fromLongBits(endBlockNumber);

        Log.d(TAG, String.format("BRCryptoCWMGenGetTransactionsCallback (%s -> %s)", begBlockNumberUnsigned, endBlockNumberUnsigned));

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            optSystem.get().query.getTransactions(coreWalletManager.getNetwork().getUids(), Collections.singletonList(address), begBlockNumberUnsigned,
                    endBlockNumberUnsigned, true,
                    false, new CompletionHandler<List<Transaction>, QueryError>() {
                        @Override
                        public void handleData(List<Transaction> transactions) {
                            Log.d(TAG, "BRCryptoCWMGenGetTransactionsCallback  received transactions");

                            for (Transaction transaction : transactions) {
                                Optional<byte[]> optRaw = transaction.getRaw();
                                if (!optRaw.isPresent()) {
                                    Log.e(TAG, "BRCryptoCWMGenGetTransactionsCallback  completing with missing raw bytes");
                                    coreWalletManager.announceGetTransactionsComplete(callbackState, false);
                                    return;
                                }

                                UnsignedLong blockHeight = transaction.getBlockHeight().or(UnsignedLong.ZERO);
                                UnsignedLong timestamp =
                                        transaction.getTimestamp().transform(Date::getTime).transform(TimeUnit.MILLISECONDS::toSeconds).transform(UnsignedLong::valueOf).or(UnsignedLong.ZERO);
                                Log.d(TAG,
                                        "BRCryptoCWMGenGetTransactionsCallback  announcing " + transaction.getId());
                                coreWalletManager.announceGetTransactionsItemGen(callbackState, optRaw.get(), timestamp, blockHeight);
                            }

                            Log.d(TAG, "BRCryptoCWMGenGetTransactionsCallback : complete");
                            coreWalletManager.announceGetTransactionsComplete(callbackState, true);
                        }

                        @Override
                        public void handleError(QueryError error) {
                            Log.e(TAG, "BRCryptoCWMGenGetTransactionsCallback  received an error, completing with failure", error);
                            coreWalletManager.announceGetTransactionsComplete(callbackState, false);
                        }
                    });

        } else {
            Log.e(TAG, "BRCryptoCWMGenGetTransactionsCallback : missing system");
            coreWalletManager.announceGetTransactionsComplete(callbackState, false);
        }
    }

    private static void genSubmitTransaction(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                             Pointer tx, SizeT txLength, String hashAsHex) {
        Log.d(TAG, "BRCryptoCWMGenSubmitTransactionCallback");

        CoreBRCryptoWalletManager coreWalletManager = CoreBRCryptoWalletManager.createOwned(manager);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            byte[] txBytes = tx.getByteArray(0, UnsignedInts.checkedCast(txLength.longValue()));

            optSystem.get().query.createTransaction(coreWalletManager.getNetwork().getUids(), hashAsHex, txBytes, new CompletionHandler<Void, QueryError>() {
                @Override
                public void handleData(Void data) {
                    Log.d(TAG, "BRCryptoCWMGenSubmitTransactionCallback: succeeded");
                    coreWalletManager.announceSubmitTransferSuccess(callbackState);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BRCryptoCWMGenSubmitTransactionCallback: failed", error);
                    coreWalletManager.announceSubmitTransferFailure(callbackState);
                }
            });

        } else {
            Log.e(TAG, "BRCryptoCWMGenSubmitTransactionCallback: missing system");
            coreWalletManager.announceSubmitTransferFailure(callbackState);
        }
    }
}
