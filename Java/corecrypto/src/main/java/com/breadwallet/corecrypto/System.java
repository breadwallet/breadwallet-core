/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.crypto.BRCryptoCWMClient;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientBtc;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientCallbackState;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientEth;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientGen;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener;
import com.breadwallet.corenative.crypto.BRCryptoStatus;
import com.breadwallet.corenative.crypto.BRCryptoTransfer;
import com.breadwallet.corenative.crypto.BRCryptoTransferEvent;
import com.breadwallet.corenative.crypto.BRCryptoTransferStateType;
import com.breadwallet.corenative.crypto.BRCryptoWallet;
import com.breadwallet.corenative.crypto.BRCryptoWalletEvent;
import com.breadwallet.corenative.crypto.BRCryptoWalletManager;
import com.breadwallet.corenative.crypto.BRCryptoWalletManagerEvent;
import com.breadwallet.corenative.support.BRConstants;
import com.breadwallet.corenative.utility.Cookie;
import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.TransferState;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletManagerState;
import com.breadwallet.crypto.WalletManagerSyncDepth;
import com.breadwallet.crypto.WalletManagerSyncStoppedReason;
import com.breadwallet.crypto.WalletState;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.BlockchainFee;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.breadwallet.crypto.blockchaindb.models.brd.EthToken;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;
import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.errors.MigrateBlockError;
import com.breadwallet.crypto.errors.MigrateCreateError;
import com.breadwallet.crypto.errors.MigrateError;
import com.breadwallet.crypto.errors.MigrateInvalidError;
import com.breadwallet.crypto.errors.MigratePeerError;
import com.breadwallet.crypto.errors.MigrateTransactionError;
import com.breadwallet.crypto.errors.NetworkFeeUpdateError;
import com.breadwallet.crypto.errors.NetworkFeeUpdateFeesUnavailableError;
import com.breadwallet.crypto.events.network.NetworkCreatedEvent;
import com.breadwallet.crypto.events.network.NetworkEvent;
import com.breadwallet.crypto.events.network.NetworkFeesUpdatedEvent;
import com.breadwallet.crypto.events.system.SystemCreatedEvent;
import com.breadwallet.crypto.events.system.SystemDiscoveredNetworksEvent;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.system.SystemManagerAddedEvent;
import com.breadwallet.crypto.events.system.SystemNetworkAddedEvent;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferChangedEvent;
import com.breadwallet.crypto.events.transfer.TransferCreatedEvent;
import com.breadwallet.crypto.events.transfer.TransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletBalanceUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.wallet.WalletFeeBasisUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferAddedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferSubmittedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerBlockUpdatedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerChangedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerCreatedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerDeletedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncProgressEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncRecommendedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStartedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStoppedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletAddedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletChangedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletDeletedEvent;
import com.breadwallet.crypto.migration.BlockBlob;
import com.breadwallet.crypto.migration.PeerBlob;
import com.breadwallet.crypto.migration.TransactionBlob;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;
import com.google.common.collect.Collections2;
import com.google.common.collect.ImmutableCollection;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArraySet;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.logging.Level;
import java.util.logging.Logger;

import static com.google.common.base.Preconditions.checkState;

/* package */
final class System implements com.breadwallet.crypto.System {

    private static final Logger Log = Logger.getLogger(System.class.getName());

    /// A index to globally identify systems.
    private static final AtomicInteger SYSTEM_IDS = new AtomicInteger(0);

    /// A dictionary mapping an index to a system.
    private static final Map<Cookie, System> SYSTEMS_ACTIVE = new ConcurrentHashMap<>();

    /// An array of removed systems.  This is a workaround for systems that have been destroyed.
    /// We do not correctly handle 'release' and thus C-level memory issues are introduced; rather
    /// than solving those memory issues now, we'll avoid 'release' by holding a reference.
    private static final List<System> SYSTEMS_INACTIVE = Collections.synchronizedList(new ArrayList<>());

    /// If true, save removed system in the above array. Set to `false` for debugging 'release'.
    private static final boolean SYSTEMS_INACTIVE_RETAIN = true;

    // Create a dedicated executor to pump CWM events as quickly as possible
    private static final Executor EXECUTOR_LISTENER = Executors.newSingleThreadExecutor();

    // Create a dedicated executor to pump CWM callbacks. This is a separate executor
    // than the one used to handle events as they *really* need to be pumped as fast as possible.
    private static final Executor EXECUTOR_CLIENT = Executors.newSingleThreadExecutor();

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
            System::genGetTransfers,
            System::genSubmitTransaction
    );

    private static final BRCryptoCWMListener.WalletManagerEventCallback CWM_LISTENER_WALLET_MANAGER_CALLBACK = System::walletManagerEventCallback;
    private static final BRCryptoCWMListener.WalletEventCallback CWM_LISTENER_WALLET_CALLBACK = System::walletEventCallback;
    private static final BRCryptoCWMListener.TransferEventCallback CWM_LISTENER_TRANSFER_CALLBACK = System::transferEventCallback;

    private static final boolean DEFAULT_IS_NETWORK_REACHABLE = true;

    private static boolean ensurePath(String storagePath) {
        File storageFile = new File(storagePath);
        return ((storageFile.exists() || storageFile.mkdirs())
                && storageFile.isDirectory()
                && storageFile.canWrite());
    }

    /* package */
    static System create(ScheduledExecutorService executor,
                         SystemListener listener,
                         com.breadwallet.crypto.Account account,
                         boolean isMainnet,
                         String storagePath,
                         BlockchainDb query) {
        Account cryptoAccount = Account.from(account);

        storagePath = storagePath + (storagePath.endsWith(File.separator) ? "" : File.separator) + cryptoAccount.getFilesystemIdentifier();
        checkState(ensurePath(storagePath));

        Cookie context = new Cookie(SYSTEM_IDS.incrementAndGet());

        BRCryptoCWMListener cwmListener = new BRCryptoCWMListener(context,
                CWM_LISTENER_WALLET_MANAGER_CALLBACK,
                CWM_LISTENER_WALLET_CALLBACK,
                CWM_LISTENER_TRANSFER_CALLBACK);

        BRCryptoCWMClient cwmClient = new BRCryptoCWMClient(context,
                CWM_CLIENT_BTC,
                CWM_CLIENT_ETH,
                CWM_CLIENT_GEN);

        System system = new System(executor,
                listener,
                cryptoAccount,
                isMainnet,
                storagePath,
                query,
                context,
                cwmListener,
                cwmClient);

        SYSTEMS_ACTIVE.put(context, system);

        return system;
    }

    /* package */
    static Optional<com.breadwallet.crypto.blockchaindb.models.bdb.Currency> asBDBCurrency(String uids,
                                                                                           String name,
                                                                                           String code,
                                                                                           String type,
                                                                                           UnsignedInteger decimals) {
        int index = uids.indexOf(':');
        if (index == -1) return Optional.absent();

        type = type.toLowerCase(Locale.ROOT);
        if (!"erc20".equals(type) && !"native".equals(type)) return Optional.absent();

        code = code.toLowerCase(Locale.ROOT);
        String blockchainId = uids.substring(0, index);
        String address = uids.substring(index);

        return Optional.of(
                com.breadwallet.crypto.blockchaindb.models.bdb.Currency.create(
                        uids,
                        name,
                        code,
                        type,
                        blockchainId,
                        address.equals("__native__") ? null : address,
                        true,
                        Blockchains.makeCurrencyDemominationsErc20(code, decimals)
                )
        );
    }

    /* package */
    static Optional<byte[]> migrateBRCoreKeyCiphertext(com.breadwallet.crypto.Key key,
                                                       byte[] nonce12,
                                                       byte[] authenticatedData,
                                                       byte[] ciphertext) {
        return Cipher.migrateBRCoreKeyCiphertext(key, nonce12, authenticatedData, ciphertext);
    }

    /* package */
    static void wipe(com.breadwallet.crypto.System system) {
        // Safe the path to the persistent storage
        String storagePath = system.getPath();

        // Destroy the system.
        destroy(system);

        // Clear out persistent storage
        deleteRecursively(storagePath);
    }

    /* package */
    static void wipeAll(String storagePath, List<com.breadwallet.crypto.System> exemptSystems) {
        Set<String> exemptSystemPath = new HashSet<>();
        for (com.breadwallet.crypto.System sys: exemptSystems) {
            exemptSystemPath.add(sys.getPath());
        }

        File storageFile = new File(storagePath);
        File[] childFiles = storageFile.listFiles();
        if (null != childFiles) {
            for (File child : childFiles) {
                if (!exemptSystemPath.contains(child.getAbsolutePath())) {
                    deleteRecursively(child);
                }
            }
        }
    }

    private static void destroy(com.breadwallet.crypto.System system) {
        System sys = System.from(system);
        // Stop all callbacks.  This might be inconsistent with 'deleted' events.
        SYSTEMS_ACTIVE.remove(sys.context);

        // Disconnect all wallet managers
        sys.disconnectAll();

        // Stop
        sys.stopAll();

        // Register the system as inactive
        if (SYSTEMS_INACTIVE_RETAIN) {
            SYSTEMS_INACTIVE.add(sys);
        }
    }

    private static void deleteRecursively (String toDeletePath) {
        deleteRecursively(new File(toDeletePath));
    }

    private static void deleteRecursively (File toDelete) {
        if (toDelete.isDirectory()) {
            for (File child : toDelete.listFiles()) {
                deleteRecursively(child);
            }
        }

        if (toDelete.exists() && !toDelete.delete()) {
            Log.log(Level.SEVERE, "Failed to delete " + toDelete.getAbsolutePath());
        }
    }

    private static Optional<System> getSystem(Cookie context) {
        return Optional.fromNullable(SYSTEMS_ACTIVE.get(context));
    }

    private static System from(com.breadwallet.crypto.System system) {
        if (system == null) {
            return null;
        }

        if (system instanceof System) {
            return (System) system;
        }

        throw new IllegalArgumentException("Unsupported system instance");
    }

    private final ExecutorService executor;
    private final SystemListener listener;
    private final SystemCallbackCoordinator callbackCoordinator;
    private final Account account;
    private final boolean isMainnet;
    private final String storagePath;
    private final BlockchainDb query;
    private final Cookie context;
    private final BRCryptoCWMListener cwmListener;
    private final BRCryptoCWMClient cwmClient;

    private final Set<Network> networks;
    private final Set<WalletManager> walletManagers;

    private boolean isNetworkReachable;

    private System(ScheduledExecutorService executor,
                   SystemListener listener,
                   Account account,
                   boolean isMainnet,
                   String storagePath,
                   BlockchainDb query,
                   Cookie context,
                   BRCryptoCWMListener cwmListener,
                   BRCryptoCWMClient cwmClient) {
        this.executor = executor;
        this.listener = listener;
        this.callbackCoordinator = new SystemCallbackCoordinator(executor);
        this.account = account;
        this.isMainnet = isMainnet;
        this.storagePath = storagePath;
        this.query = query;
        this.context = context;
        this.cwmListener = cwmListener;
        this.cwmClient = cwmClient;

        this.networks = new CopyOnWriteArraySet<>();
        this.walletManagers = new CopyOnWriteArraySet<>();

        this.isNetworkReachable = DEFAULT_IS_NETWORK_REACHABLE;

        announceSystemEvent(new SystemCreatedEvent());
    }

    @Override
    public void configure(List<com.breadwallet.crypto.blockchaindb.models.bdb.Currency> appCurrencies) {
        NetworkDiscovery.discoverNetworks(query, isMainnet, appCurrencies, new NetworkDiscovery.Callback() {
            @Override
            public void discovered(Network network) {
                if (addNetwork(network)) {
                    announceNetworkEvent(network, new NetworkCreatedEvent());
                    announceSystemEvent(new SystemNetworkAddedEvent(network));
                }
            }

            @Override
            public void complete(List<com.breadwallet.crypto.Network> networks) {
                announceSystemEvent(new SystemDiscoveredNetworksEvent(networks));
            }
        });
    }

    @Override
    public boolean createWalletManager(com.breadwallet.crypto.Network network,
                                       WalletManagerMode mode,
                                       AddressScheme scheme,
                                       Set<com.breadwallet.crypto.Currency> currencies) {
        checkState(network.supportsWalletManagerMode(mode));
        checkState(network.supportsAddressScheme(scheme));

        Optional<WalletManager> maybeWalletManager = WalletManager.create(
                cwmListener,
                cwmClient,
                account,
                Network.from(network),
                mode,
                scheme,
                storagePath,
                this,
                callbackCoordinator);
        if (!maybeWalletManager.isPresent()) {
            return false;
        }

        WalletManager walletManager = maybeWalletManager.get();
        for (com.breadwallet.crypto.Currency currency: currencies) {
            if (network.hasCurrency(currency)) {
                walletManager.registerWalletFor(currency);
            }
        }

        walletManager.setNetworkReachable(isNetworkReachable);

        addWalletManager(walletManager);
        announceSystemEvent(new SystemManagerAddedEvent(walletManager));
        return true;
    }

    @Override
    public void wipe(com.breadwallet.crypto.Network network) {
        boolean found = false;
        for (WalletManager walletManager: walletManagers) {
            if (walletManager.getNetwork().equals(network)) {
                found = true;
                break;
            }
        }

        // Racy - but if there is no wallet manager for `network`... then
        if (!found) {
            WalletManager.wipe(Network.from(network), storagePath);
        }
    }

    @Override
    public void connectAll() {
        for (WalletManager manager: getWalletManagers()) {
            manager.connect(null);
        }
    }

    @Override
    public void disconnectAll() {
        for (WalletManager manager: getWalletManagers()) {
            manager.disconnect();
        }
    }

    @Override
    public void subscribe(String subscriptionToken) {
        // TODO(fix): Implement this!
    }

    @Override
    public void updateNetworkFees(@Nullable CompletionHandler<List<com.breadwallet.crypto.Network>, NetworkFeeUpdateError> handler) {
        query.getBlockchains(isMainnet, new CompletionHandler<List<Blockchain>, QueryError>() {
            @Override
            public void handleData(List<Blockchain> blockchainModels) {
                Map<String, Network> networksByUuid = new HashMap<>();
                for (Network network: getNetworks()) networksByUuid.put(network.getUids(), network);

                List<com.breadwallet.crypto.Network> networks = new ArrayList<>();
                for (Blockchain blockChainModel: blockchainModels) {
                    Network network = networksByUuid.get(blockChainModel.getId());
                    if (null == network) continue;

                    // We always have a feeUnit for network
                    Optional<Unit> maybeFeeUnit = network.baseUnitFor(network.getCurrency());
                    checkState(maybeFeeUnit.isPresent());

                    List<NetworkFee> fees = new ArrayList<>();
                    for (BlockchainFee feeEstimate: blockChainModel.getFeeEstimates()) {
                        // Well, quietly ignore a fee if we can't parse the amount.
                        Optional<Amount> maybeFeeAmount = Amount.create(feeEstimate.getAmount(), false, maybeFeeUnit.get());
                        if (!maybeFeeAmount.isPresent()) continue;

                        fees.add(NetworkFee.create(feeEstimate.getConfirmationTimeInMilliseconds(), maybeFeeAmount.get()));
                    }

                    // The fees are unlikely to change; but we'll announce feesUpdated anyways.
                    network.setFees(fees);
                    announceNetworkEvent(network, new NetworkFeesUpdatedEvent());
                    networks.add(network);
                }

                if (null != handler) handler.handleData(networks);
            }

            @Override
            public void handleError(QueryError error) {
                // On an error, just skip out; we'll query again later, presumably
                if (null != handler) handler.handleError(new NetworkFeeUpdateFeesUnavailableError());
            }
        });
    }

    @Override
    public void setNetworkReachable(boolean isNetworkReachable) {
        this.isNetworkReachable = isNetworkReachable;
        for (WalletManager manager: getWalletManagers()) {
            manager.setNetworkReachable(isNetworkReachable);
        }
    }

    @Override
    public Account getAccount() {
        return account;
    }

    @Override
    public String getPath() {
        return storagePath;
    }

    @Override
    public List<Wallet> getWallets() {
        List<Wallet> wallets = new ArrayList<>();
        for (WalletManager manager: getWalletManagers()) {
            wallets.addAll(manager.getWallets());
        }
        return wallets;
    }

    private void stopAll() {
        for (WalletManager manager: getWalletManagers()) {
            manager.stop();
        }
    }

    // Network management

    @Override
    public List<Network> getNetworks() {
        return new ArrayList<>(networks);
    }

    private boolean addNetwork(Network network) {
        return networks.add(network);
    }

    // WalletManager management

    @Override
    public List<WalletManager> getWalletManagers() {
        return new ArrayList<>(walletManagers);
    }

    private void addWalletManager(WalletManager walletManager) {
        walletManagers.add(walletManager);
    }

    private Optional<WalletManager> getWalletManager(BRCryptoWalletManager coreWalletManager) {
        WalletManager walletManager = WalletManager.takeAndCreate(coreWalletManager, this, callbackCoordinator);
        return walletManagers.contains(walletManager) ? Optional.of(walletManager) : Optional.absent();
    }

    private WalletManager createWalletManager(BRCryptoWalletManager coreWalletManager) {
        WalletManager walletManager = WalletManager.takeAndCreate(coreWalletManager, this, callbackCoordinator);
        walletManagers.add(walletManager);
        return walletManager;
    }

    // Miscellaneous

    @Override
    public boolean migrateRequired(com.breadwallet.crypto.Network network) {
        return network.requiresMigration();
    }

    @Override
    public void migrateStorage (com.breadwallet.crypto.Network network,
                                List<TransactionBlob> transactionBlobs,
                                List<BlockBlob> blockBlobs,
                                List<PeerBlob> peerBlobs) throws MigrateError {
        if (!migrateRequired(network)) {
            throw new MigrateInvalidError();
        }

         migrateStorageAsBtc(network, transactionBlobs, blockBlobs, peerBlobs);
    }

    private void migrateStorageAsBtc (com.breadwallet.crypto.Network network,
                                      List<TransactionBlob> transactionBlobs,
                                      List<BlockBlob> blockBlobs,
                                      List<PeerBlob> peerBlobs) throws MigrateError {
        Optional<WalletMigrator> maybeMigrator = WalletMigrator.create(network, storagePath);
        if (!maybeMigrator.isPresent()) {
            throw new MigrateCreateError();
        }

        WalletMigrator migrator = maybeMigrator.get();

        try {
            for (TransactionBlob blob : transactionBlobs) {
                Optional<TransactionBlob.Btc> maybeBtc = blob.asBtc();
                if (!maybeBtc.isPresent()) {
                    throw new MigrateTransactionError();
                }

                TransactionBlob.Btc btc = maybeBtc.get();

                if (!migrator.handleTransactionAsBtc(
                        btc.bytes,
                        btc.blockHeight,
                        btc.timestamp)) {
                    throw new MigrateTransactionError();
                }
            }

            for (BlockBlob blob : blockBlobs) {
                Optional<BlockBlob.Btc> maybeBtc = blob.asBtc();
                if (!maybeBtc.isPresent()) {
                    throw new MigrateBlockError();
                }

                BlockBlob.Btc btc = maybeBtc.get();

                if (!migrator.handleBlockAsBtc(
                        btc.block,
                        btc.height)) {
                    throw new MigrateBlockError();
                }
            }

            for (PeerBlob blob : peerBlobs) {
                Optional<PeerBlob.Btc> maybeBtc = blob.asBtc();
                if (!maybeBtc.isPresent()) {
                    throw new MigratePeerError();
                }

                PeerBlob.Btc btc = maybeBtc.get();
                // On a `nil` timestamp, by definition skip out, don't migrate this blob
                if (btc.timestamp == null) continue;

                if (!migrator.handlePeerAsBtc(
                        btc.address,
                        btc.port,
                        btc.services,
                        btc.timestamp)) {
                    throw new MigratePeerError();
                }
            }
        } finally {
            migrator.release();
        }
    }

    /* package */
    BlockchainDb getBlockchainDb() {
        return query;
    }

    // Event announcements

    private void announceSystemEvent(SystemEvent event) {
        executor.submit(() -> listener.handleSystemEvent(this, event));
    }

    private void announceNetworkEvent(Network network, NetworkEvent event) {
        executor.submit(() -> listener.handleNetworkEvent(this, network, event));
    }

    private void announceWalletManagerEvent(WalletManager walletManager, WalletManagerEvent event) {
        executor.submit(() -> listener.handleManagerEvent(this, walletManager, event));
    }

    private void announceWalletEvent(WalletManager walletManager, Wallet wallet, WalletEvent event) {
        executor.submit(() -> listener.handleWalletEvent(this, walletManager, wallet, event));
    }

    private void announceTransferEvent(WalletManager walletManager, Wallet wallet, Transfer transfer, TranferEvent event) {
        executor.submit(() -> listener.handleTransferEvent(this, walletManager, wallet, transfer, event));
    }

    //
    // WalletManager Events
    //

    private static void walletManagerEventCallback(Cookie context,
                                                   BRCryptoWalletManager coreWalletManager,
                                                   BRCryptoWalletManagerEvent event) {
        EXECUTOR_LISTENER.execute(() -> {
            try {
                Log.log(Level.FINE, "WalletManagerEventCallback");

                switch (event.type()) {
                    case CRYPTO_WALLET_MANAGER_EVENT_CREATED: {
                        handleWalletManagerCreated(context, coreWalletManager);
                        break;
                    }
                    case CRYPTO_WALLET_MANAGER_EVENT_CHANGED: {
                        handleWalletManagerChanged(context, coreWalletManager, event);
                        break;
                    }
                    case CRYPTO_WALLET_MANAGER_EVENT_DELETED: {
                        handleWalletManagerDeleted(context, coreWalletManager);
                        break;
                    }
                    case CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED: {
                        handleWalletManagerWalletAdded(context, coreWalletManager, event);
                        break;
                    }
                    case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED: {
                        handleWalletManagerWalletChanged(context, coreWalletManager, event);
                        break;
                    }
                    case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED: {
                        handleWalletManagerWalletDeleted(context, coreWalletManager, event);
                        break;
                    }
                    case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED: {
                        handleWalletManagerSyncStarted(context, coreWalletManager);
                        break;
                    }
                    case CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES: {
                        handleWalletManagerSyncProgress(context, coreWalletManager, event);
                        break;
                    }
                    case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED: {
                        handleWalletManagerSyncStopped(context, coreWalletManager, event);
                        break;
                    }
                    case CRYPTO_WALLET_MANAGER_EVENT_SYNC_RECOMMENDED: {
                        handleWalletManagerSyncRecommended(context, coreWalletManager, event);
                        break;
                    }
                    case CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED: {
                        handleWalletManagerBlockHeightUpdated(context, coreWalletManager, event);
                        break;
                    }
                }
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void handleWalletManagerCreated(Cookie context, BRCryptoWalletManager coreWalletManager) {
        Log.log(Level.FINE, "WalletManagerCreated");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            WalletManager walletManager = system.createWalletManager(coreWalletManager);
            system.announceWalletManagerEvent(walletManager, new WalletManagerCreatedEvent());

        } else {
            Log.log(Level.SEVERE, "WalletManagerCreated: missed system");
        }
    }

    private static void handleWalletManagerChanged(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        WalletManagerState oldState = Utilities.walletManagerStateFromCrypto(event.u.state.oldValue);
        WalletManagerState newState = Utilities.walletManagerStateFromCrypto(event.u.state.newValue);

        Log.log(Level.FINE, String.format("WalletManagerChanged (%s -> %s)", oldState, newState));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.announceWalletManagerEvent(walletManager, new WalletManagerChangedEvent(oldState, newState));

            } else {
                Log.log(Level.SEVERE, "WalletManagerChanged: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletManagerChanged: missed system");
        }
    }

    private static void handleWalletManagerDeleted(Cookie context, BRCryptoWalletManager coreWalletManager) {
        Log.log(Level.FINE, "WalletManagerDeleted");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.announceWalletManagerEvent(walletManager, new WalletManagerDeletedEvent());

            } else {
                Log.log(Level.SEVERE, "WalletManagerDeleted: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletManagerDeleted: missed system");
        }
    }

    private static void handleWalletManagerWalletAdded(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        BRCryptoWallet coreWallet = event.u.wallet.value;
        try {
            Log.log(Level.FINE, "WalletManagerWalletAdded");

            Optional<System> optSystem = getSystem(context);
            if (optSystem.isPresent()) {
                System system = optSystem.get();

                Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                if (optWalletManager.isPresent()) {
                    WalletManager walletManager = optWalletManager.get();

                    Optional<Wallet> optional = walletManager.getWallet(coreWallet);
                    if (optional.isPresent()) {
                        Wallet wallet = optional.get();
                        system.announceWalletManagerEvent(walletManager, new WalletManagerWalletAddedEvent(wallet));

                    } else {
                        Log.log(Level.SEVERE, "WalletManagerWalletAdded: missed wallet");
                    }

                } else {
                    Log.log(Level.SEVERE, "WalletManagerWalletAdded: missed wallet manager");
                }

            } else {
                Log.log(Level.SEVERE, "WalletManagerWalletAdded: missed system");
            }

        } finally {
            coreWallet.give();
        }
    }

    private static void handleWalletManagerWalletChanged(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        BRCryptoWallet coreWallet = event.u.wallet.value;
        try {
            Log.log(Level.FINE, "WalletManagerWalletChanged");

            Optional<System> optSystem = getSystem(context);
            if (optSystem.isPresent()) {
                System system = optSystem.get();

                Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                if (optWalletManager.isPresent()) {
                    WalletManager walletManager = optWalletManager.get();

                    Optional<Wallet> optional = walletManager.getWallet(coreWallet);
                    if (optional.isPresent()) {
                        Wallet wallet = optional.get();
                        system.announceWalletManagerEvent(walletManager, new WalletManagerWalletChangedEvent(wallet));

                    } else {
                        Log.log(Level.SEVERE, "WalletManagerWalletChanged: missed wallet");
                    }

                } else {
                    Log.log(Level.SEVERE, "WalletManagerWalletChanged: missed wallet manager");
                }

            } else {
                Log.log(Level.SEVERE, "WalletManagerWalletChanged: missed system");
            }

        } finally {
            coreWallet.give();
        }
    }

    private static void handleWalletManagerWalletDeleted(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        BRCryptoWallet coreWallet = event.u.wallet.value;
        try {
            Log.log(Level.FINE, "WalletManagerWalletDeleted");

            Optional<System> optSystem = getSystem(context);
            if (optSystem.isPresent()) {
                System system = optSystem.get();

                Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                if (optWalletManager.isPresent()) {
                    WalletManager walletManager = optWalletManager.get();

                    Optional<Wallet> optional = walletManager.getWallet(coreWallet);
                    if (optional.isPresent()) {
                        Wallet wallet = optional.get();
                        system.announceWalletManagerEvent(walletManager, new WalletManagerWalletDeletedEvent(wallet));

                    } else {
                        Log.log(Level.SEVERE, "WalletManagerWalletDeleted: missed wallet");
                    }

                } else {
                    Log.log(Level.SEVERE, "WalletManagerWalletDeleted: missed wallet manager");
                }

            } else {
                Log.log(Level.SEVERE, "WalletManagerWalletDeleted: missed system");
            }

        } finally {
            coreWallet.give();
        }
    }

    private static void handleWalletManagerSyncStarted(Cookie context, BRCryptoWalletManager coreWalletManager) {
        Log.log(Level.FINE, "WalletManagerSyncStarted");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.announceWalletManagerEvent(walletManager, new WalletManagerSyncStartedEvent());

            } else {
                Log.log(Level.SEVERE, "WalletManagerSyncStarted: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletManagerSyncStarted: missed system");
        }
    }

    private static void handleWalletManagerSyncProgress(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        float percent = event.u.syncContinues.percentComplete;
        Date timestamp = 0 == event.u.syncContinues.timestamp ? null : new Date(TimeUnit.SECONDS.toMillis(event.u.syncContinues.timestamp));

        Log.log(Level.FINE, String.format("WalletManagerSyncProgress (%s)", percent));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.announceWalletManagerEvent(walletManager, new WalletManagerSyncProgressEvent(percent, timestamp));

            } else {
                Log.log(Level.SEVERE, "WalletManagerSyncProgress: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletManagerSyncProgress: missed system");
        }
    }

    private static void handleWalletManagerSyncStopped(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        WalletManagerSyncStoppedReason reason = Utilities.walletManagerSyncStoppedReasonFromCrypto(event.u.syncStopped.reason);
        Log.log(Level.FINE, String.format("WalletManagerSyncStopped: (%s)", reason));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.announceWalletManagerEvent(walletManager, new WalletManagerSyncStoppedEvent(reason));

            } else {
                Log.log(Level.SEVERE, "WalletManagerSyncStopped: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletManagerSyncStopped: missed system");
        }
    }

    private static void handleWalletManagerSyncRecommended(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        WalletManagerSyncDepth depth = Utilities.syncDepthFromCrypto(event.u.syncRecommended.depth());
        Log.log(Level.FINE, String.format("WalletManagerSyncRecommended: (%s)", depth));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.announceWalletManagerEvent(walletManager, new WalletManagerSyncRecommendedEvent(depth));

            } else {
                Log.log(Level.SEVERE, "WalletManagerSyncRecommended: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletManagerSyncRecommended: missed system");
        }
    }

    private static void handleWalletManagerBlockHeightUpdated(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWalletManagerEvent event) {
        UnsignedLong blockHeight = UnsignedLong.fromLongBits(event.u.blockHeight.value);

        Log.log(Level.FINE, String.format("WalletManagerBlockHeightUpdated (%s)", blockHeight));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();
                system.announceWalletManagerEvent(walletManager, new WalletManagerBlockUpdatedEvent(blockHeight));

            } else {
                Log.log(Level.SEVERE, "WalletManagerBlockHeightUpdated: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletManagerBlockHeightUpdated: missed system");
        }
    }

    //
    // Wallet Events
    //

    private static void walletEventCallback(Cookie context,
                                            BRCryptoWalletManager coreWalletManager,
                                            BRCryptoWallet coreWallet,
                                            BRCryptoWalletEvent event) {
        EXECUTOR_LISTENER.execute(() -> {
            try {
                Log.log(Level.FINE, "WalletEventCallback");

                switch (event.type()) {
                    case CRYPTO_WALLET_EVENT_CREATED: {
                        handleWalletCreated(context, coreWalletManager, coreWallet);
                        break;
                    }
                    case CRYPTO_WALLET_EVENT_CHANGED: {
                        handleWalletChanged(context, coreWalletManager, coreWallet, event);
                        break;
                    }
                    case CRYPTO_WALLET_EVENT_DELETED: {
                        handleWalletDeleted(context, coreWalletManager, coreWallet);
                        break;
                    }
                    case CRYPTO_WALLET_EVENT_TRANSFER_ADDED: {
                        handleWalletTransferAdded(context, coreWalletManager, coreWallet, event);
                        break;
                    }
                    case CRYPTO_WALLET_EVENT_TRANSFER_CHANGED: {
                        handleWalletTransferChanged(context, coreWalletManager, coreWallet, event);
                        break;
                    }
                    case CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED: {
                        handleWalletTransferSubmitted(context, coreWalletManager, coreWallet, event);
                        break;
                    }
                    case CRYPTO_WALLET_EVENT_TRANSFER_DELETED: {
                        handleWalletTransferDeleted(context, coreWalletManager, coreWallet, event);
                        break;
                    }
                    case CRYPTO_WALLET_EVENT_BALANCE_UPDATED: {
                        handleWalletBalanceUpdated(context, coreWalletManager, coreWallet, event);
                        break;
                    }
                    case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED: {
                        handleWalletFeeBasisUpdated(context, coreWalletManager, coreWallet, event);
                        break;
                    }
                    case CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED: {
                        handleWalletFeeBasisEstimated(context, event);
                        break;
                    }
                }
            } finally {
                coreWallet.give();
                coreWalletManager.give();
            }
        });
    }

    private static void handleWalletCreated(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet) {
        Log.log(Level.FINE, "WalletCreated");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Wallet wallet = walletManager.createWallet(coreWallet);
                system.announceWalletEvent(walletManager, wallet, new WalletCreatedEvent());

            } else {
                Log.log(Level.SEVERE, "WalletCreated: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletCreated: missed system");
        }
    }

    private static void handleWalletChanged(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        WalletState oldState = Utilities.walletStateFromCrypto(event.u.state.oldState());
        WalletState newState = Utilities.walletStateFromCrypto(event.u.state.newState());

        Log.log(Level.FINE, String.format("WalletChanged (%s -> %s)", oldState, newState));

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    system.announceWalletEvent(walletManager, wallet, new WalletChangedEvent(oldState, newState));

                } else {
                    Log.log(Level.SEVERE, "WalletChanged: missed wallet");
                }

            } else {
                Log.log(Level.SEVERE, "WalletChanged: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletChanged: missed system");
        }
    }

    private static void handleWalletDeleted(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet) {
        Log.log(Level.FINE, "WalletDeleted");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();
                    system.announceWalletEvent(walletManager, wallet, new WalletDeletedEvent());

                } else {
                    Log.log(Level.SEVERE, "WalletDeleted: missed wallet");
                }

            } else {
                Log.log(Level.SEVERE, "WalletDeleted: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletDeleted: missed system");
        }
    }

    private static void handleWalletTransferAdded(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        BRCryptoTransfer coreTransfer = event.u.transfer.value;
        try {
            Log.log(Level.FINE, "WalletTransferAdded");

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
                            system.announceWalletEvent(walletManager, wallet, new WalletTransferAddedEvent(transfer));

                        } else {
                            Log.log(Level.SEVERE, "WalletTransferAdded: missed transfer");
                        }

                    } else {
                        Log.log(Level.SEVERE, "WalletTransferAdded: missed wallet");
                    }

                } else {
                    Log.log(Level.SEVERE, "WalletTransferAdded: missed wallet manager");
                }

            } else {
                Log.log(Level.SEVERE, "WalletTransferAdded: missed system");
            }
        } finally {
            coreTransfer.give();
        }
    }

    private static void handleWalletTransferChanged(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        BRCryptoTransfer coreTransfer = event.u.transfer.value;
        try {
            Log.log(Level.FINE, "WalletTransferChanged");

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
                            system.announceWalletEvent(walletManager, wallet, new WalletTransferChangedEvent(transfer));

                        } else {
                            Log.log(Level.SEVERE, "WalletTransferChanged: missed transfer");
                        }

                    } else {
                        Log.log(Level.SEVERE, "WalletTransferChanged: missed wallet");
                    }

                } else {
                    Log.log(Level.SEVERE, "WalletTransferChanged: missed wallet manager");
                }

            } else {
                Log.log(Level.SEVERE, "WalletTransferChanged: missed system");
            }
        } finally {
            coreTransfer.give();
        }
    }

    private static void handleWalletTransferSubmitted(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        BRCryptoTransfer coreTransfer = event.u.transfer.value;
        try {
            Log.log(Level.FINE, "WalletTransferSubmitted");

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
                            system.announceWalletEvent(walletManager, wallet, new WalletTransferSubmittedEvent(transfer));

                        } else {
                            Log.log(Level.SEVERE, "WalletTransferSubmitted: missed transfer");
                        }

                    } else {
                        Log.log(Level.SEVERE, "WalletTransferSubmitted: missed wallet");
                    }

                } else {
                    Log.log(Level.SEVERE, "WalletTransferSubmitted: missed wallet manager");
                }

            } else {
                Log.log(Level.SEVERE, "WalletTransferSubmitted: missed system");
            }
        } finally {
            coreTransfer.give();
        }
    }

    private static void handleWalletTransferDeleted(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        BRCryptoTransfer coreTransfer = event.u.transfer.value;
        try {
            Log.log(Level.FINE, "WalletTransferDeleted");

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
                            system.announceWalletEvent(walletManager, wallet, new WalletTransferDeletedEvent(transfer));

                        } else {
                            Log.log(Level.SEVERE, "WalletTransferDeleted: missed transfer");
                        }

                    } else {
                        Log.log(Level.SEVERE, "WalletTransferDeleted: missed wallet");
                    }

                } else {
                    Log.log(Level.SEVERE, "WalletTransferDeleted: missed wallet manager");
                }

            } else {
                Log.log(Level.SEVERE, "WalletTransferDeleted: missed system");
            }
        } finally {
            coreTransfer.give();
        }
    }

    private static void handleWalletBalanceUpdated(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.log(Level.FINE, "WalletBalanceUpdated");

        Amount amount = Amount.create(event.u.balanceUpdated.amount);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();

                    Log.log(Level.FINE, String.format("WalletBalanceUpdated: %s", amount));
                    system.announceWalletEvent(walletManager, wallet, new WalletBalanceUpdatedEvent(amount));

                } else {
                    Log.log(Level.SEVERE, "WalletBalanceUpdated: missed wallet");
                }

            } else {
                Log.log(Level.SEVERE, "WalletBalanceUpdated: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletBalanceUpdated: missed system");
        }
    }

    private static void handleWalletFeeBasisUpdated(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.log(Level.FINE, "WalletFeeBasisUpdate");

        TransferFeeBasis feeBasis = TransferFeeBasis.create(event.u.feeBasisUpdated.basis);

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();

                    Log.log(Level.FINE, String.format("WalletFeeBasisUpdate: %s", feeBasis));
                    system.announceWalletEvent(walletManager, wallet, new WalletFeeBasisUpdatedEvent(feeBasis));

                } else {
                    Log.log(Level.SEVERE, "WalletFeeBasisUpdate: missed wallet");
                }

            } else {
                Log.log(Level.SEVERE, "WalletFeeBasisUpdate: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "WalletFeeBasisUpdate: missed system");
        }
    }

    private static void handleWalletFeeBasisEstimated(Cookie context, BRCryptoWalletEvent event) {
        BRCryptoStatus status = event.u.feeBasisEstimated.status();

        Log.log(Level.FINE, String.format("WalletFeeBasisEstimated (%s)", status));

        boolean success = status == BRCryptoStatus.CRYPTO_SUCCESS;
        TransferFeeBasis feeBasis = success ? TransferFeeBasis.create(event.u.feeBasisEstimated.basis) : null;

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();
            Cookie opCookie = new Cookie(event.u.feeBasisEstimated.cookie);

            if (success) {
                Log.log(Level.FINE, String.format("WalletFeeBasisEstimated: %s", feeBasis));
                system.callbackCoordinator.completeFeeBasisEstimateHandlerWithSuccess(opCookie, feeBasis);
            } else {
                FeeEstimationError error = Utilities.feeEstimationErrorFromStatus(status);
                Log.log(Level.FINE, String.format("WalletFeeBasisEstimated: %s", error));
                system.callbackCoordinator.completeFeeBasisEstimateHandlerWithError(opCookie, error);
            }

        } else {
            Log.log(Level.SEVERE, "WalletFeeBasisEstimated: missed system");
        }
    }

    //
    // Transfer Events
    //

    private static void transferEventCallback(Cookie context,
                                              BRCryptoWalletManager coreWalletManager,
                                              BRCryptoWallet coreWallet,
                                              BRCryptoTransfer coreTransfer,
                                              BRCryptoTransferEvent event) {
        EXECUTOR_LISTENER.execute(() -> {
            try {
                Log.log(Level.FINE, "TransferEventCallback");

                switch (event.type()) {
                    case CRYPTO_TRANSFER_EVENT_CREATED: {
                        handleTransferCreated(context, coreWalletManager, coreWallet, coreTransfer);
                        break;
                    }
                    case CRYPTO_TRANSFER_EVENT_CHANGED: {
                        handleTransferChanged(context, coreWalletManager, coreWallet, coreTransfer, event);
                        break;
                    }
                    case CRYPTO_TRANSFER_EVENT_DELETED: {
                        handleTransferDeleted(context, coreWalletManager, coreWallet, coreTransfer);
                        break;
                    }
                }
            } finally {
                coreTransfer.give();
                coreWallet.give();
                coreWalletManager.give();
            }
        });
    }

    private static void handleTransferCreated(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet, BRCryptoTransfer coreTransfer) {
        Log.log(Level.FINE, "TransferCreated");

        Optional<System> optSystem = getSystem(context);
        if (optSystem.isPresent()) {
            System system = optSystem.get();

            Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
            if (optWalletManager.isPresent()) {
                WalletManager walletManager = optWalletManager.get();

                Optional<Wallet> optWallet = walletManager.getWallet(coreWallet);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();

                    Transfer transfer = wallet.createTransfer(coreTransfer);
                    system.announceTransferEvent(walletManager, wallet, transfer, new TransferCreatedEvent());

                } else {
                    Log.log(Level.SEVERE, "TransferCreated: missed wallet");
                }

            } else {
                Log.log(Level.SEVERE, "TransferCreated: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "TransferCreated: missed system");
        }
    }

    private static void handleTransferChanged(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet, BRCryptoTransfer coreTransfer,
                                       BRCryptoTransferEvent event) {
        TransferState oldState = Utilities.transferStateFromCrypto(event.u.state.oldState);
        TransferState newState = Utilities.transferStateFromCrypto(event.u.state.newState);

        Log.log(Level.FINE, String.format("TransferChanged (%s -> %s)", oldState, newState));

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

                        system.announceTransferEvent(walletManager, wallet, transfer, new TransferChangedEvent(oldState, newState));

                    } else {
                        Log.log(Level.SEVERE, "TransferChanged: missed transfer");
                    }

                } else {
                    Log.log(Level.SEVERE, "TransferChanged: missed wallet");
                }

            } else {
                Log.log(Level.SEVERE, "TransferChanged: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "TransferChanged: missed system");
        }
    }

    private static void handleTransferDeleted(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoWallet coreWallet, BRCryptoTransfer coreTransfer) {
        Log.log(Level.FINE, "TransferDeleted");

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
                        system.announceTransferEvent(walletManager, wallet, transfer, new TransferDeletedEvent());

                    } else {
                        Log.log(Level.SEVERE, "TransferDeleted: missed transfer");
                    }

                } else {
                    Log.log(Level.SEVERE, "TransferDeleted: missed wallet");
                }

            } else {
                Log.log(Level.SEVERE, "TransferDeleted: missed wallet manager");
            }

        } else {
            Log.log(Level.SEVERE, "TransferDeleted: missed system");
        }
    }

    // BTC client

    private static void btcGetBlockNumber(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMBtcGetBlockNumberCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getBlockchain(walletManager.getNetwork().getUids(), new CompletionHandler<Blockchain, QueryError>() {
                            @Override
                            public void handleData(Blockchain blockchain) {
                                Optional<UnsignedLong> maybeBlockHeight = blockchain.getBlockHeight();
                                if (maybeBlockHeight.isPresent()) {
                                    UnsignedLong blockchainHeight = maybeBlockHeight.get();
                                    Log.log(Level.FINE, String.format("BRCryptoCWMBtcGetBlockNumberCallback: succeeded (%s)", blockchainHeight));
                                    walletManager.getCoreBRCryptoWalletManager().announceGetBlockNumberSuccess(callbackState, blockchainHeight);
                                } else {
                                    Log.log(Level.SEVERE, "BRCryptoCWMBtcGetBlockNumberCallback: failed with missing block height");
                                    walletManager.getCoreBRCryptoWalletManager().announceGetBlockNumberFailure(callbackState);
                                }
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMBtcGetBlockNumberCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceGetBlockNumberFailure(callbackState);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMBtcGetBlockNumberCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMBtcGetBlockNumberCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetBlockNumberFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void btcGetTransactions(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                    List<String> addresses, long begBlockNumber, long endBlockNumber) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                UnsignedLong begBlockNumberUnsigned = UnsignedLong.fromLongBits(begBlockNumber);
                UnsignedLong endBlockNumberUnsigned = UnsignedLong.fromLongBits(endBlockNumber);

                Log.log(Level.FINE, String.format("BRCryptoCWMBtcGetTransactionsCallback (%s -> %s)", begBlockNumberUnsigned, endBlockNumberUnsigned));

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getTransactions(walletManager.getNetwork().getUids(),
                                addresses,
                                begBlockNumberUnsigned.equals(BRConstants.BLOCK_HEIGHT_UNBOUND) ? null : begBlockNumberUnsigned,
                                endBlockNumberUnsigned.equals(BRConstants.BLOCK_HEIGHT_UNBOUND) ? null : endBlockNumberUnsigned,
                                true,
                                false,
                                new CompletionHandler<List<Transaction>, QueryError>() {
                                    @Override
                                    public void handleData(List<Transaction> transactions) {
                                        boolean success = false;
                                        Log.log(Level.FINE, "BRCryptoCWMBtcGetTransactionsCallback received transactions");

                                        try {
                                            for (Transaction transaction : transactions) {
                                                Optional<byte[]> optRaw = transaction.getRaw();
                                                if (!optRaw.isPresent()) {
                                                    Log.log(Level.SEVERE, "BRCryptoCWMBtcGetTransactionsCallback completing with missing raw bytes");
                                                    return;
                                                }

                                                UnsignedLong blockHeight = transaction.getBlockHeight().or(UnsignedLong.ZERO);
                                                UnsignedLong timestamp =
                                                        transaction.getTimestamp().transform(Utilities::dateAsUnixTimestamp).or(UnsignedLong.ZERO);
                                                BRCryptoTransferStateType status = (transaction.getStatus().equals("confirmed")
                                                        ? BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_INCLUDED
                                                        : (transaction.getStatus().equals("submitted")
                                                        ? BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_SUBMITTED
                                                        : (transaction.getStatus().equals("failed")
                                                        ? BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_ERRORED
                                                        : BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_DELETED)));  // Query API error

                                                if (status != BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_DELETED) {
                                                    Log.log(Level.FINE,
                                                            "BRCryptoCWMBtcGetTransactionsCallback announcing " + transaction.getId());
                                                    walletManager.getCoreBRCryptoWalletManager().announceGetTransactionsItemBtc(callbackState, status, optRaw.get(), timestamp, blockHeight);
                                                } else {
                                                    Log.log(Level.SEVERE, "BRCryptoCWMBtcGetTransactionsCallback received an unknown status, completing with failure");
                                                    return;
                                                }
                                            }

                                            success = true;
                                            Log.log(Level.FINE, "BRCryptoCWMBtcGetTransactionsCallback: complete");
                                        } finally {
                                            walletManager.getCoreBRCryptoWalletManager().announceGetTransactionsComplete(callbackState, success);
                                        }
                                    }

                                    @Override
                                    public void handleError(QueryError error) {
                                        Log.log(Level.SEVERE, "BRCryptoCWMBtcGetTransactionsCallback received an error, completing with failure", error);
                                        walletManager.getCoreBRCryptoWalletManager().announceGetTransactionsComplete(callbackState, false);
                                    }
                                });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMBtcGetTransactionsCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMBtcGetTransactionsCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetTransactionsComplete(callbackState, false);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void btcSubmitTransaction(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                             byte[] transaction, String hashAsHex) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMBtcSubmitTransactionCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.createTransaction(walletManager.getNetwork().getUids(), hashAsHex, transaction, new CompletionHandler<Void, QueryError>() {
                            @Override
                            public void handleData(Void data) {
                                Log.log(Level.FINE, "BRCryptoCWMBtcSubmitTransactionCallback: succeeded");
                                walletManager.getCoreBRCryptoWalletManager().announceSubmitTransferSuccess(callbackState);
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMBtcSubmitTransactionCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceSubmitTransferFailure(callbackState);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMBtcSubmitTransactionCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMBtcSubmitTransactionCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceSubmitTransferFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    // ETH client

    private static void ethGetEtherBalance(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                    String networkName, String address) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMEthGetEtherBalanceCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getBalanceAsEth(networkName, address, new CompletionHandler<String, QueryError>() {
                            @Override
                            public void handleData(String balance) {
                                Log.log(Level.FINE, "BRCryptoCWMEthGetEtherBalanceCallback: succeeded");
                                walletManager.getCoreBRCryptoWalletManager().announceGetBalanceSuccess(callbackState, balance);
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMEthGetEtherBalanceCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceGetBalanceFailure(callbackState);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMEthGetEtherBalanceCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMEthGetEtherBalanceCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetBalanceFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void ethGetTokenBalance(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                    String networkName, String address, String tokenAddress) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMEthGetTokenBalanceCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getBalanceAsTok(networkName, address, tokenAddress, new CompletionHandler<String, QueryError>() {
                            @Override
                            public void handleData(String balance) {
                                Log.log(Level.FINE, "BRCryptoCWMEthGetTokenBalanceCallback: succeeded");
                                walletManager.getCoreBRCryptoWalletManager().announceGetBalanceSuccess(callbackState, balance);
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMEthGetTokenBalanceCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceGetBalanceFailure(callbackState);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMEthGetTokenBalanceCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMEthGetTokenBalanceCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetBalanceFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void ethGetGasPrice(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                String networkName) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMEthGetGasPriceCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getGasPriceAsEth(networkName, new CompletionHandler<String, QueryError>() {
                            @Override
                            public void handleData(String gasPrice) {
                                Log.log(Level.FINE, "BRCryptoCWMEthGetGasPriceCallback: succeeded");
                                walletManager.getCoreBRCryptoWalletManager().announceGetGasPriceSuccess(callbackState, gasPrice);
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMEthGetGasPriceCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceGetGasPriceFailure(callbackState);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMEthGetGasPriceCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMEthGetGasPriceCallback: missing sytem");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetGasPriceFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void ethEstimateGas(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                String networkName, String from, String to, String amount, String gasPrice, String data) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMEthEstimateGasCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getGasEstimateAsEth(networkName, from, to, amount, data, new CompletionHandler<String, QueryError>() {
                            @Override
                            public void handleData(String gasEstimate) {
                                Log.log(Level.FINE, "BRCryptoCWMEthEstimateGasCallback: succeeded");
                                walletManager.getCoreBRCryptoWalletManager().announceGetGasEstimateSuccess(callbackState, gasEstimate, gasPrice);
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMEthEstimateGasCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceGetGasEstimateFailure(callbackState, BRCryptoStatus.CRYPTO_ERROR_NODE_NOT_CONNECTED);
                            }
                        });

                    } else {
                        // using the CRYPTO_ERROR_FAILED status code as this represents a situation where the system that this estimation
                        // was queued for, is now GC'ed. As a result, no one is really listening for this estimation so return an error
                        // code indicating failure and leave it at that.
                        throw new IllegalStateException("BRCryptoCWMEthEstimateGasCallback: missing manager");
                    }

                } else {
                    // using the CRYPTO_ERROR_FAILED status code as this represents a situation where the system that this estimation
                    // was queued for, is now GC'ed. As a result, no one is really listening for this estimation so return an error
                    // code indicating failure and leave it at that.
                    throw new IllegalStateException("BRCryptoCWMEthEstimateGasCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetGasEstimateFailure(callbackState, BRCryptoStatus.CRYPTO_ERROR_FAILED);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void ethSubmitTransaction(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                      String networkName, String transaction) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMEthSubmitTransactionCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.submitTransactionAsEth(networkName, transaction, new CompletionHandler<String, QueryError>() {
                            @Override
                            public void handleData(String hash) {
                                Log.log(Level.FINE, "BRCryptoCWMEthSubmitTransactionCallback: succeeded");
                                walletManager.getCoreBRCryptoWalletManager().announceSubmitTransferSuccess(callbackState, hash);
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMEthSubmitTransactionCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceSubmitTransferFailure(callbackState);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMEthSubmitTransactionCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMEthSubmitTransactionCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceSubmitTransferFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void ethGetTransactions(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                    String networkName, String address, long begBlockNumber, long endBlockNumber) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, String.format("BRCryptoCWMEthGetTransactionsCallback (%s -> %s)", begBlockNumber, endBlockNumber));

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getTransactionsAsEth(networkName, address, UnsignedLong.fromLongBits(begBlockNumber),
                                UnsignedLong.fromLongBits(endBlockNumber), new CompletionHandler<List<EthTransaction>, QueryError>() {
                                    @Override
                                    public void handleData(List<EthTransaction> transactions) {
                                        boolean success = false;
                                        Log.log(Level.FINE, "BRCryptoCWMEthGetTransactionsCallback: succeeded");

                                        try {
                                            for (EthTransaction tx : transactions) {
                                                walletManager.getCoreBRCryptoWalletManager().announceGetTransactionsItemEth(
                                                        callbackState,
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
                                                        tx.getBlockTransactionIndex(),
                                                        tx.getBlockTimestamp(),
                                                        tx.getIsError());
                                            }

                                            success = true;
                                        } finally {
                                            walletManager.getCoreBRCryptoWalletManager().announceGetTransactionsComplete(callbackState, success);
                                        }
                                    }

                                    @Override
                                    public void handleError(QueryError error) {
                                        Log.log(Level.SEVERE, "BRCryptoCWMEthGetTransactionsCallback: failed", error);
                                        walletManager.getCoreBRCryptoWalletManager().announceGetTransactionsComplete(callbackState, false);
                                    }
                                });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMEthGetTransactionsCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMEthGetTransactionsCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetTransactionsComplete(callbackState, false);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void ethGetLogs(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                            String networkName, String contract, String address, String event, long begBlockNumber,
                            long endBlockNumber) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, String.format("BRCryptoCWMEthGetLogsCallback (%s -> %s)", begBlockNumber, endBlockNumber));

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getLogsAsEth(networkName, contract, address, event, UnsignedLong.fromLongBits(begBlockNumber),
                                UnsignedLong.fromLongBits(endBlockNumber), new CompletionHandler<List<EthLog>, QueryError>() {
                                    @Override
                                    public void handleData(List<EthLog> logs) {
                                        boolean success = false;
                                        Log.log(Level.FINE, "BRCryptoCWMEthGetLogsCallback: succeeded");

                                        try {
                                            for (EthLog log : logs) {
                                                walletManager.getCoreBRCryptoWalletManager().announceGetLogsItem(
                                                        callbackState,
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

                                            success = true;
                                        } finally {
                                            walletManager.getCoreBRCryptoWalletManager().announceGetLogsComplete(callbackState, success);
                                        }
                                    }

                                    @Override
                                    public void handleError(QueryError error) {
                                        Log.log(Level.SEVERE, "BRCryptoCWMEthGetLogsCallback: failed", error);
                                        walletManager.getCoreBRCryptoWalletManager().announceGetLogsComplete(callbackState, false);
                                    }
                                });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMEthGetLogsCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMEthGetLogsCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetLogsComplete(callbackState, false);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void ethGetBlocks(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                              String networkName, String address, int interests, long blockNumberStart,
                              long blockNumberStop) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMEthGetBlocksCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getBlocksAsEth(networkName, address, UnsignedInteger.fromIntBits(interests),
                                UnsignedLong.fromLongBits(blockNumberStart), UnsignedLong.fromLongBits(blockNumberStop),
                                new CompletionHandler<List<UnsignedLong>, QueryError>() {
                                    @Override
                                    public void handleData(List<UnsignedLong> blocks) {
                                        Log.log(Level.FINE, "BRCryptoCWMEthGetBlocksCallback: succeeded");
                                        walletManager.getCoreBRCryptoWalletManager().announceGetBlocksSuccess(callbackState, blocks);
                                    }

                                    @Override
                                    public void handleError(QueryError error) {
                                        Log.log(Level.SEVERE, "BRCryptoCWMEthGetBlocksCallback: failed", error);
                                        walletManager.getCoreBRCryptoWalletManager().announceGetBlocksFailure(callbackState);
                                    }
                                });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMEthGetBlocksCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMEthGetBlocksCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetBlocksFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void ethGetTokens(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BREthereumClientHandlerGetTokens");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getTokensAsEth(new CompletionHandler<List<EthToken>, QueryError>() {
                            @Override
                            public void handleData(List<EthToken> tokens) {
                                boolean success = false;
                                Log.log(Level.FINE, "BREthereumClientHandlerGetTokens: succeeded");

                                try {
                                    for (EthToken token : tokens) {
                                        walletManager.getCoreBRCryptoWalletManager().announceGetTokensItem(
                                                callbackState,
                                                token.getAddress(),
                                                token.getSymbol(),
                                                token.getName(),
                                                token.getDescription(),
                                                token.getDecimals(),
                                                token.getDefaultGasLimit().orNull(),
                                                token.getDefaultGasPrice().orNull());
                                    }

                                    success = true;
                                } finally {
                                    walletManager.getCoreBRCryptoWalletManager().announceGetTokensComplete(callbackState, success);
                                }
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BREthereumClientHandlerGetTokens: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceGetTokensComplete(callbackState, false);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BREthereumClientHandlerGetTokens: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BREthereumClientHandlerGetTokens: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetTokensComplete(callbackState, false);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void ethGetBlockNumber(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                   String networkName) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMEthGetBlockNumberCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getBlockNumberAsEth(networkName, new CompletionHandler<String, QueryError>() {
                            @Override
                            public void handleData(String number) {
                                Log.log(Level.FINE, "BRCryptoCWMEthGetBlockNumberCallback: succeeded");
                                walletManager.getCoreBRCryptoWalletManager().announceGetBlockNumberSuccess(callbackState, number);
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMEthGetBlockNumberCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceGetBlockNumberFailure(callbackState);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMEthGetBlockNumberCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMEthGetBlockNumberCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetBlockNumberFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void ethGetNonce(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                             String networkName, String address) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMEthGetNonceCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getNonceAsEth(networkName, address, new CompletionHandler<String, QueryError>() {
                            @Override
                            public void handleData(String nonce) {
                                Log.log(Level.FINE, "BRCryptoCWMEthGetNonceCallback: succeeded");
                                walletManager.getCoreBRCryptoWalletManager().announceGetNonceSuccess(callbackState, address, nonce);
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMEthGetNonceCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceGetNonceFailure(callbackState);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMEthGetNonceCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMEthGetNonceCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetNonceFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    // GEN client

    private static void genGetBlockNumber(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMGenGetBlockNumberCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getBlockchain(walletManager.getNetwork().getUids(), new CompletionHandler<Blockchain, QueryError>() {
                            @Override
                            public void handleData(Blockchain blockchain) {
                                Optional<UnsignedLong> maybeBlockHeight = blockchain.getBlockHeight();
                                if (maybeBlockHeight.isPresent()) {
                                    UnsignedLong blockchainHeight = maybeBlockHeight.get();
                                    Log.log(Level.FINE, String.format("BRCryptoCWMGenGetBlockNumberCallback: succeeded (%s)", blockchainHeight));
                                    walletManager.getCoreBRCryptoWalletManager().announceGetBlockNumberSuccess(callbackState, blockchainHeight);
                                } else {
                                    Log.log(Level.SEVERE, "BRCryptoCWMGenGetBlockNumberCallback: failed with missing block height");
                                    walletManager.getCoreBRCryptoWalletManager().announceGetBlockNumberFailure(callbackState);
                                }
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMGenGetBlockNumberCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceGetBlockNumberFailure(callbackState);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMGenGetBlockNumberCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMGenGetBlockNumberCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetBlockNumberFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void genGetTransactions(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                           String address, long begBlockNumber, long endBlockNumber) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                UnsignedLong begBlockNumberUnsigned = UnsignedLong.fromLongBits(begBlockNumber);
                UnsignedLong endBlockNumberUnsigned = UnsignedLong.fromLongBits(endBlockNumber);

                Log.log(Level.FINE, String.format("BRCryptoCWMGenGetTransactionsCallback (%s -> %s)", begBlockNumberUnsigned, endBlockNumberUnsigned));

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getTransactions(
                                walletManager.getNetwork().getUids(),
                                Collections.singletonList(address),
                                begBlockNumberUnsigned.equals(BRConstants.BLOCK_HEIGHT_UNBOUND) ? null : begBlockNumberUnsigned,
                                endBlockNumberUnsigned.equals(BRConstants.BLOCK_HEIGHT_UNBOUND) ? null : endBlockNumberUnsigned,
                                true,
                                false,
                                new CompletionHandler<List<Transaction>, QueryError>() {
                                    @Override
                                    public void handleData(List<Transaction> transactions) {
                                        boolean success = false;
                                        Log.log(Level.FINE, "BRCryptoCWMGenGetTransactionsCallback  received transactions");

                                        try {
                                            for (Transaction transaction : transactions) {
                                                Optional<byte[]> optRaw = transaction.getRaw();
                                                if (!optRaw.isPresent()) {
                                                    Log.log(Level.SEVERE, "BRCryptoCWMGenGetTransactionsCallback  completing with missing raw bytes");
                                                    return;
                                                }

                                                UnsignedLong blockHeight = transaction.getBlockHeight().or(UnsignedLong.ZERO);
                                                UnsignedLong timestamp =
                                                        transaction.getTimestamp().transform(Utilities::dateAsUnixTimestamp).or(UnsignedLong.ZERO);

                                                BRCryptoTransferStateType status = (transaction.getStatus().equals("confirmed")
                                                        ? BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_INCLUDED
                                                        : (transaction.getStatus().equals("submitted")
                                                        ? BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_SUBMITTED
                                                        : (transaction.getStatus().equals("failed")
                                                        ? BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_ERRORED
                                                        : BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_DELETED))); // Query API error

                                                if (status != BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_DELETED) {
                                                    Log.log(Level.FINE,
                                                            "BRCryptoCWMGenGetTransactionsCallback  announcing " + transaction.getId());
                                                    walletManager.getCoreBRCryptoWalletManager().announceGetTransactionsItemGen(callbackState, status, optRaw.get(), timestamp, blockHeight);
                                                } else {
                                                    Log.log(Level.SEVERE, "BRCryptoCWMGenGetTransactionsCallback received an unknown status, completing with failure");
                                                    return;
                                                }
                                            }

                                            success = true;
                                            Log.log(Level.FINE, "BRCryptoCWMGenGetTransactionsCallback : complete");
                                        } finally {
                                            walletManager.getCoreBRCryptoWalletManager().announceGetTransactionsComplete(callbackState, success);
                                        }
                                    }

                                    @Override
                                    public void handleError(QueryError error) {
                                        Log.log(Level.SEVERE, "BRCryptoCWMGenGetTransactionsCallback  received an error, completing with failure", error);
                                        walletManager.getCoreBRCryptoWalletManager().announceGetTransactionsComplete(callbackState, false);
                                    }
                                });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMGenGetTransactionsCallback : missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMGenGetTransactionsCallback : missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetTransactionsComplete(callbackState, false);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static void genGetTransfers(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                           String address, long begBlockNumber, long endBlockNumber) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                UnsignedLong begBlockNumberUnsigned = UnsignedLong.fromLongBits(begBlockNumber);
                UnsignedLong endBlockNumberUnsigned = UnsignedLong.fromLongBits(endBlockNumber);

                Log.log(Level.FINE, String.format("BRCryptoCWMGenGetTransfersCallback (%s -> %s)", begBlockNumberUnsigned, endBlockNumberUnsigned));

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.getTransactions(walletManager.getNetwork().getUids(), Collections.singletonList(address), begBlockNumberUnsigned,
                                endBlockNumberUnsigned, false,
                                false, new CompletionHandler<List<Transaction>, QueryError>() {
                                    @Override
                                    public void handleData(List<Transaction> transactions) {
                                        boolean success = false;
                                        Log.log(Level.FINE, "BRCryptoCWMGenGetTransfersCallback received transfers");
                                        List<ObjectPair<com.breadwallet.crypto.blockchaindb.models.bdb.Transfer, String>> merged;

                                        try {
                                            for (Transaction transaction : transactions) {
                                                UnsignedLong blockHeight = transaction.getBlockHeight().or(UnsignedLong.ZERO);
                                                UnsignedLong timestamp = transaction.getTimestamp().transform(Utilities::dateAsUnixTimestamp).or(UnsignedLong.ZERO);

                                                BRCryptoTransferStateType status = (transaction.getStatus().equals("confirmed")
                                                        ? BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_INCLUDED
                                                        : (transaction.getStatus().equals("submitted")
                                                        ? BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_SUBMITTED
                                                        : (transaction.getStatus().equals("failed")
                                                        ? BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_ERRORED
                                                        : BRCryptoTransferStateType.CRYPTO_TRANSFER_STATE_DELETED))); // Query API error

                                                merged = System.mergeTransfers(transaction, address);
                                                for (ObjectPair<com.breadwallet.crypto.blockchaindb.models.bdb.Transfer, String> o : merged) {
                                                    Log.log(Level.FINE, "BRCryptoCWMGenGetTransfersCallback  announcing " + o.o1.getId());
                                                    walletManager.getCoreBRCryptoWalletManager().announceGetTransfersItemGen(callbackState, status,
                                                            transaction.getHash(),
                                                            o.o1.getId(),
                                                            o.o1.getFromAddress().orNull(),
                                                            o.o1.getToAddress().orNull(),
                                                            o.o1.getAmount().getAmount(),
                                                            o.o1.getAmount().getCurrencyId(),
                                                            o.o2,
                                                            timestamp,
                                                            blockHeight,
                                                            o.o1.getMeta());
                                                }
                                            }

                                            success = true;
                                            Log.log(Level.FINE, "BRCryptoCWMGenGetTransfersCallback : complete");
                                        } finally {
                                            walletManager.getCoreBRCryptoWalletManager().announceGetTransfersComplete(callbackState, success);
                                        }
                                    }

                                    @Override
                                    public void handleError(QueryError error) {
                                        Log.log(Level.SEVERE, "BRCryptoCWMGenGetTransfersCallback  received an error, completing with failure", error);
                                        walletManager.getCoreBRCryptoWalletManager().announceGetTransfersComplete(callbackState, false);
                                    }
                                });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMGenGetTransfersCallback : missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMGenGetTransfersCallback : missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceGetTransfersComplete(callbackState, false);
            } finally {
                coreWalletManager.give();
            }
        });
    }

    private static class ObjectPair<T1, T2> {
        final T1 o1;
        final T2 o2;

        ObjectPair (T1 o1, T2 o2) {
            this.o1 = o1;
            this.o2 = o2;
        }
    }

    private static List<ObjectPair<com.breadwallet.crypto.blockchaindb.models.bdb.Transfer, String>> mergeTransfers(Transaction transaction, String address) {
        List<com.breadwallet.crypto.blockchaindb.models.bdb.Transfer> transfers;
        List<com.breadwallet.crypto.blockchaindb.models.bdb.Transfer> transfersWithFee;
        List<com.breadwallet.crypto.blockchaindb.models.bdb.Transfer> transfersWithoutFee;
        List<ObjectPair<com.breadwallet.crypto.blockchaindb.models.bdb.Transfer, String>> transfersMerged;
        com.breadwallet.crypto.blockchaindb.models.bdb.Transfer transferWithFee;

        // Only consider transfers w/ `address`
        transfers = new ArrayList<>(Collections2.filter(transaction.getTransfers(), t -> address.equals(t.getFromAddress().orNull()) || address.equals(t.getToAddress().orNull())));

        // Note for later: all transfers have a unique id

        transfersWithFee = new ArrayList<>(Collections2.filter(transfers, t -> "__fee__".equals(t.getToAddress().orNull())));
        transfersWithoutFee = new ArrayList<>(Collections2.filter(transfers, t -> !"__fee__".equals(t.getToAddress().orNull())));

        // Get the transferWithFee if we have one
        checkState(transfersWithFee.size() <= 1);
        transferWithFee = transfersWithFee.isEmpty() ? null : transfersWithFee.get(0);

        transfersMerged = new ArrayList<>(transfers.size());

        // There is no "__fee__" entry
        if (transferWithFee == null) {
            // Announce transfers with no fee
            for (com.breadwallet.crypto.blockchaindb.models.bdb.Transfer transfer: transfers) {
                transfersMerged.add(new ObjectPair<>(transfer, null));
            }

        // There is a single "__fee__" entry, due to `checkState(transfersWithFee.size() <= 1)` above
        } else {
            // We may or may not have a non-fee transfer matching `transferWithFee`.  We
            // may or may not have more than one non-fee transfers matching `transferWithFee`

            // Find the first of the non-fee transfers matching `transferWithFee`
            com.breadwallet.crypto.blockchaindb.models.bdb.Transfer transferMatchingFee = null;
            for (com.breadwallet.crypto.blockchaindb.models.bdb.Transfer transfer: transfersWithoutFee) {
                if (transferWithFee.getTransactionId().equals(transfer.getTransactionId()) &&
                    transferWithFee.getFromAddress().equals(transfer.getFromAddress())) {
                    transferMatchingFee = transfer;
                    break;
                }
            }

            // We must have a transferMatchingFee; if we don't add one
            transfers = new ArrayList<>(transfersWithoutFee);
            if (null == transferMatchingFee) {
                transfers.add(
                        com.breadwallet.crypto.blockchaindb.models.bdb.Transfer.create(
                                transferWithFee.getId(),
                                transferWithFee.getBlockchainId(),
                                transferWithFee.getIndex(),
                                transferWithFee.getAmount(),
                                transferWithFee.getMeta(),
                                transferWithFee.getFromAddress().orNull(),
                                "unknown",
                                "0",
                                transferWithFee.getAcknowledgements().orNull())
                );
            }

            // Hold the Id for the transfer that we'll add a fee to.
            String transferForFeeId = transferMatchingFee != null ? transferMatchingFee.getId() : transferWithFee.getId();

            // Announce transfers adding the fee to the `transferforFeeId`
            for (com.breadwallet.crypto.blockchaindb.models.bdb.Transfer transfer: transfers) {
                String fee = transfer.getId().equals(transferForFeeId) ? transferWithFee.getAmount().getAmount() : null;

                transfersMerged.add(new ObjectPair<>(transfer, fee));
            }
        }

        return transfersMerged;
    }

    private static void genSubmitTransaction(Cookie context, BRCryptoWalletManager coreWalletManager, BRCryptoCWMClientCallbackState callbackState,
                                             byte[] transaction, String hashAsHex) {
        EXECUTOR_CLIENT.execute(() -> {
            try {
                Log.log(Level.FINE, "BRCryptoCWMGenSubmitTransactionCallback");

                Optional<System> optSystem = getSystem(context);
                if (optSystem.isPresent()) {
                    System system = optSystem.get();

                    Optional<WalletManager> optWalletManager = system.getWalletManager(coreWalletManager);
                    if (optWalletManager.isPresent()) {
                        WalletManager walletManager = optWalletManager.get();

                        system.query.createTransaction(walletManager.getNetwork().getUids(), hashAsHex, transaction, new CompletionHandler<Void, QueryError>() {
                            @Override
                            public void handleData(Void data) {
                                Log.log(Level.FINE, "BRCryptoCWMGenSubmitTransactionCallback: succeeded");
                                walletManager.getCoreBRCryptoWalletManager().announceSubmitTransferSuccess(callbackState);
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.log(Level.SEVERE, "BRCryptoCWMGenSubmitTransactionCallback: failed", error);
                                walletManager.getCoreBRCryptoWalletManager().announceSubmitTransferFailure(callbackState);
                            }
                        });

                    } else {
                        throw new IllegalStateException("BRCryptoCWMGenSubmitTransactionCallback: missing manager");
                    }

                } else {
                    throw new IllegalStateException("BRCryptoCWMGenSubmitTransactionCallback: missing system");
                }
            } catch (RuntimeException e) {
                Log.log(Level.SEVERE, e.getMessage());
                coreWalletManager.announceSubmitTransferFailure(callbackState);
            } finally {
                coreWalletManager.give();
            }
        });
    }
}
