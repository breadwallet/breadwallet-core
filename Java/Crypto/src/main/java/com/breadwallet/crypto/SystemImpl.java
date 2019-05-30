package com.breadwallet.crypto;

import android.util.Log;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.events.network.NetworkCreatedEvent;
import com.breadwallet.crypto.events.network.NetworkEvent;
import com.breadwallet.crypto.events.system.SystemCreatedEvent;
import com.breadwallet.crypto.events.system.SystemEvent;
import com.breadwallet.crypto.events.system.SystemListener;
import com.breadwallet.crypto.events.system.SystemManagerAddedEvent;
import com.breadwallet.crypto.events.system.SystemNetworkAddedEvent;
import com.breadwallet.crypto.events.transfer.TranferEvent;
import com.breadwallet.crypto.events.transfer.TransferChangedEvent;
import com.breadwallet.crypto.events.transfer.TransferConfirmationEvent;
import com.breadwallet.crypto.events.transfer.TransferCreatedEvent;
import com.breadwallet.crypto.events.transfer.TransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletBalanceUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferAddedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferSubmittedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerChangedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerCreatedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStartedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStoppedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletChangedEvent;
import com.breadwallet.crypto.jni.bitcoin.BRTransaction;
import com.breadwallet.crypto.jni.bitcoin.BRTransactionEvent;
import com.breadwallet.crypto.jni.bitcoin.BRWallet;
import com.breadwallet.crypto.jni.bitcoin.BRWalletEvent;
import com.breadwallet.crypto.jni.bitcoin.BRWalletManager;
import com.breadwallet.crypto.jni.bitcoin.BRWalletManagerClient;
import com.breadwallet.crypto.jni.bitcoin.BRWalletManagerEvent;
import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.bitcoin.CoreBRTransaction;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

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

    private final List<Network> networks = Collections.synchronizedList(new ArrayList<>());
    private final List<WalletManager> walletManagers = Collections.synchronizedList(new ArrayList<>());
    private final ExecutorService systemExecutor = Executors.newSingleThreadExecutor();

    // TODO: Ownership, should this be owned by WalletManagerImplBtc?
    private final BRWalletManagerClient.ByValue coreBitcoinClient = new BRWalletManagerClient.ByValue(Pointer.NULL,
            this::doGetBlockNumber,
            this::doGetTransactions,
            this::doSubmitTransation,
            this::handleTransactionEvent,
            this::handleWalletEvent,
            this::handleWalletManagerEvent);

    private final ExecutorService listenerExecutor;
    private final WeakReference<SystemListener> listener;
    private final Account account;
    private final String path;
    private final BlockchainDb query;

    private SystemImpl(ExecutorService listenerExecutor, SystemListener listener, Account account, String path, BlockchainDb query) {
        checkState(INSTANCE == null);

        this.listenerExecutor = listenerExecutor;
        this.listener = new WeakReference<>(listener);
        this.account = account;
        this.path = path;
        this.query = query;

        announceSystemEvent(new SystemCreatedEvent());
    }

    // Public Interface

    @Override
    public void subscribe(String subscriptionToken) {
        // TODO: Implement this!
    }

    @Override
    public void initialize(List<String> networksNeeded) {
        NetworkDiscovery.discoverNetworks(query, networksNeeded, networks -> {
            for (Network network: networks) {
                systemExecutor.submit(() -> addNetwork(network));
            }
        });
    }

    @Override
    public void createWalletManager(Network network, WalletManagerMode mode) {
        String networkCode = network.getCurrency().getCode();
        if (networkCode.equals(com.breadwallet.crypto.Currency.CODE_AS_BTC)) {
            systemExecutor.submit(() -> {
                WalletManagerBtcImpl walletManager = new WalletManagerBtcImpl(coreBitcoinClient, account, network, mode, path);
                addWalletManager(walletManager);
            });
        } else {
            // TODO: How do we want to handle this?
        }
    }

    @Override
    public Optional<SystemListener> getSystemListener() {
        return Optional.fromNullable(listener.get());
    }

    @Override
    public Account getAccount() {
        return account;
    }

    @Override
    public String getPath() {
        return path;
    }

    // Event Publishers

    private void announceSystemEvent(SystemEvent event) {
        SystemListener listener = this.listener.get();
        if (null != listener) {
            listenerExecutor.submit(() -> listener.handleSystemEvent(this, event));
        }
    }

    private void announceNetworkEvent(Network network, NetworkEvent event) {
        SystemListener listener = this.listener.get();
        if (null != listener) {
            listenerExecutor.submit(() -> listener.handleNetworkEvent(this, network, event));
        }
    }

    private void announceWalletManagerEvent(WalletManager walletManager, WalletManagerEvent event) {
        SystemListener listener = this.listener.get();
        if (null != listener) {
            listenerExecutor.submit(() -> listener.handleManagerEvent(this, walletManager, event));
        }
    }

    private void announceWalletEvent(WalletManager walletManager, Wallet wallet, WalletEvent event) {
        SystemListener listener = this.listener.get();
        if (null != listener) {
            listenerExecutor.submit(() -> listener.handleWalletEvent(this, walletManager, wallet, event));
        }
    }

    private void announceTransferEvent(WalletManager walletManager, Wallet wallet, Transfer transfer, TranferEvent event) {
        SystemListener listener = this.listener.get();
        if (null != listener) {
            listenerExecutor.submit(() -> listener.handleTransferEvent(this, walletManager, wallet, transfer, event));
        }
    }

    // BTC

    // TODO: Split each case handler into a method

    private void doGetBlockNumber(Pointer context, BRWalletManager managerImpl, int rid) {
        systemExecutor.submit(() -> {
            Optional<WalletManager> optManager = getWalletManagerByImpl(managerImpl);
            if (optManager.isPresent()) {
                WalletManager walletManager = optManager.get();

                Log.d(TAG, "BRGetBlockNumberCallback");
                query.getBlockchain(walletManager.getNetwork().getUids(), new BlockchainCompletionHandler<Blockchain>() {
                    @Override
                    public void handleData(Blockchain blockchain) {
                        long blockchainHeight = blockchain.getBlockHeight();
                        Log.d(TAG, String.format("BRGetBlockNumberCallback: succeeded with block number %s", blockchainHeight));
                        walletManager.announceBlockNumber(rid, blockchainHeight);
                    }

                    @Override
                    public void handleError(QueryError error) {
                        Log.d(TAG, "BRGetBlockNumberCallback: failed", error);
                    }
                });

            } else {
                Log.d(TAG, "BRGetBlockNumberCallback: missed manager");
            }
        });
    }

    private void doGetTransactions(Pointer context, BRWalletManager managerImpl, long begBlockNumber, long endBlockNumber, int rid) {
        // TODO: Test all this; backend blocking this working
        systemExecutor.submit(() -> {
            Optional<WalletManager> optManager = getWalletManagerByImpl(managerImpl);
            if (optManager.isPresent()) {
                WalletManager walletManager = optManager.get();
                String blockchainId = walletManager.getNetwork().getUids();

                Log.d(TAG, "BRGetTransactionsCallback");
                query.getTransactions(blockchainId, walletManager.getUnusedAddrsLegacy(25), begBlockNumber, endBlockNumber, true, false, new BlockchainCompletionHandler<List<Transaction>>() {
                    @Override
                    public void handleData(List<Transaction> transactions) {
                        Log.d(TAG, "BRGetTransactionsCallback: transaction success");
                        for (Transaction transaction: transactions) {
                            Optional<byte[]> optRaw = transaction.getRaw();
                            if (!optRaw.isPresent()) {
                                walletManager.announceTransactionComplete(rid, false);
                                return;
                            }

                            // TODO: Are these casts safe?

                            long timestamp = transaction.getTimestamp().transform((ts) -> ts.getTime() / 1000).or(0L);
                            if (timestamp < 0 || timestamp > Integer.MAX_VALUE) {
                                walletManager.announceTransactionComplete(rid, false);
                                return;
                            }

                            long blockHeight = transaction.getBlockHeight().or(0L);
                            if (blockHeight < 0 || blockHeight > Integer.MAX_VALUE) {
                                walletManager.announceTransactionComplete(rid, false);
                                return;
                            }

                            Optional<CoreBRTransaction> optCore = CoreBRTransaction.create(optRaw.get(), (int) timestamp, (int) blockHeight);
                            if (!optCore.isPresent()) {
                                walletManager.announceTransactionComplete(rid, false);
                                return;
                            }

                            walletManager.announceTransaction(rid, optCore.get());
                        }

                        if (transactions.isEmpty()) {
                            walletManager.announceTransactionComplete(rid, true);
                        } else {
                            query.getTransactions(blockchainId, walletManager.getUnusedAddrsLegacy(25), begBlockNumber, endBlockNumber, true, false, this);
                        }
                    }

                    @Override
                    public void handleError(QueryError error) {
                        Log.d(TAG, "BRGetTransactionsCallback: transaction failed", error);
                        walletManager.announceTransactionComplete(rid, false);
                    }
                });
            } else {
                Log.d(TAG, "BRGetTransactionsCallback: missed manager");
            }
        });
    }

    private void doSubmitTransation(Pointer context, BRWalletManager managerImpl, BRWallet walletImpl, BRTransaction transactionImpl, int rid) {
        // TODO: Test all this; backend blocking doGetTransactions() which is blocking ability to submit transactions
        systemExecutor.submit(() -> {
            Optional<WalletManager> optManager = getWalletManagerByImpl(managerImpl);
            if (optManager.isPresent()) {
                WalletManager walletManager = optManager.get();

                Optional<Wallet> optWallet = walletManager.getOrCreateWalletByImpl(walletImpl, false);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();

                    Optional<Transfer> optTransfer = wallet.getOrCreateTransferByImpl(transactionImpl, false);
                    if (optTransfer.isPresent()) {
                        Transfer transfer = optTransfer.get();

                        Log.d(TAG, "BRSubmitTransactionCallback");
                        query.putTransaction(walletManager.getNetwork().getUids(), transfer.serialize(), new BlockchainCompletionHandler<Transaction>() {
                            @Override
                            public void handleData(Transaction data) {
                                Log.d(TAG, "BRSubmitTransactionCallback: succeeded");
                                walletManager.announceSubmit(rid, transfer, 0);
                            }

                            @Override
                            public void handleError(QueryError error) {
                                Log.d(TAG, "BRSubmitTransactionCallback: failed", error);
                                walletManager.announceSubmit(rid, transfer, 1);
                            }
                        });

                    } else {
                        Log.d(TAG, "BRSubmitTransactionCallback: missed transfer");
                    }

                } else {
                    Log.d(TAG, "BRSubmitTransactionCallback: missed wallet");
                }

            } else {
                Log.d(TAG, "BRSubmitTransactionCallback: missed manager");
            }
        });
    }

    private void handleTransactionEvent(Pointer context, BRWalletManager managerImpl, BRWallet walletImpl, BRTransaction transactionImpl, BRTransactionEvent.ByValue event) {
        systemExecutor.submit(() -> {
            Optional<WalletManager> optManager = getWalletManagerByImpl(managerImpl);
            if (optManager.isPresent()) {
                WalletManager walletManager = optManager.get();

                Optional<Wallet> optWallet = walletManager.getOrCreateWalletByImpl(walletImpl, false);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();

                    boolean createAllowed = event.type == CryptoLibrary.BRTransactionEventType.BITCOIN_TRANSACTION_ADDED;
                    Optional<Transfer> optTransfer = wallet.getOrCreateTransferByImpl(transactionImpl, createAllowed);
                    if (optTransfer.isPresent()) {
                        Transfer transfer = optTransfer.get();

                        switch (event.type) {
                            case CryptoLibrary.BRTransactionEventType.BITCOIN_TRANSACTION_ADDED:
                                Log.d(TAG, "BRTransactionEventCallback: BITCOIN_TRANSACTION_ADDED");

                                announceTransferEvent(walletManager, wallet, transfer, new TransferCreatedEvent());
                                announceWalletEvent(walletManager, wallet, new WalletTransferAddedEvent(transfer));
                                break;
                            case CryptoLibrary.BRTransactionEventType.BITCOIN_TRANSACTION_DELETED:
                                Log.d(TAG, "BRTransactionEventCallback: BITCOIN_TRANSACTION_DELETED");

                                announceTransferEvent(walletManager, wallet, transfer, new TransferDeletedEvent());
                                announceWalletEvent(walletManager, wallet, new WalletTransferDeletedEvent(transfer));
                                break;
                            case CryptoLibrary.BRTransactionEventType.BITCOIN_TRANSACTION_UPDATED:
                                int blockHeight = event.u.updated.blockHeight;
                                int timestamp = event.u.updated.timestamp;
                                Log.d(TAG, String.format("BRTransactionEventCallback: BITCOIN_TRANSACTION_UPDATED (%s, %s)", blockHeight, timestamp));

                                Optional<Amount> optionalAmount = Amount.create(0, walletManager.getDefaultUnit());
                                if (optionalAmount.isPresent()) {
                                    TransferConfirmation confirmation = new TransferConfirmation(blockHeight, 0, timestamp, optionalAmount.get());

                                    TransferState newState = TransferState.createIncluded(confirmation);
                                    TransferState oldState = transfer.setState(newState);

                                    announceTransferEvent(walletManager, wallet, transfer, new TransferChangedEvent(oldState, newState));
                                }

                                break;
                            default:
                                throw new IllegalStateException(String.format("Unsupported BRTransactionEventCallback type: %s", event.type));
                        }

                    } else {
                        Log.d(TAG, String.format("BRTransactionEventCallback: missed transfer for type: %s", event.type));
                    }

                } else {
                    Log.d(TAG, String.format("BRTransactionEventCallback: missed wallet for type: %s", event.type));
                }

            } else {
                Log.d(TAG, String.format("BRTransactionEventCallback: missed manager for type: %s", event.type));
            }

        });
    }

    private void handleWalletEvent(Pointer context, BRWalletManager managerImpl, BRWallet walletImpl, BRWalletEvent.ByValue event) {
        systemExecutor.submit(() -> {
            Optional<WalletManager> optManager = getWalletManagerByImpl(managerImpl);
            if (optManager.isPresent()) {
                WalletManager walletManager = optManager.get();

                boolean createAllowed = event.type == CryptoLibrary.BRWalletEventType.BITCOIN_WALLET_CREATED;
                Optional<Wallet> optWallet = walletManager.getOrCreateWalletByImpl(walletImpl, createAllowed);
                if (optWallet.isPresent()) {
                    Wallet wallet = optWallet.get();

                    switch (event.type) {
                        case CryptoLibrary.BRWalletEventType.BITCOIN_WALLET_CREATED: {
                            Log.d(TAG, "BRWalletEventCallback: BITCOIN_WALLET_CREATED");

                            announceWalletEvent(walletManager, wallet, new WalletCreatedEvent());
                            break;
                        }
                        case CryptoLibrary.BRWalletEventType.BITCOIN_WALLET_DELETED: {
                            Log.d(TAG, "BRWalletEventCallback: BITCOIN_WALLET_DELETED");

                            announceWalletEvent(walletManager, wallet, new WalletDeletedEvent());
                            break;
                        }
                        case CryptoLibrary.BRWalletEventType.BITCOIN_WALLET_BALANCE_UPDATED: {
                            Log.d(TAG, String.format("BRWalletEventCallback: BITCOIN_WALLET_BALANCE_UPDATED (%s)", event.u.balance.satoshi));

                            announceWalletEvent(walletManager, wallet, new WalletBalanceUpdatedEvent(wallet.getBalance()));
                            announceWalletManagerEvent(walletManager, new WalletManagerWalletChangedEvent(wallet));
                            break;
                        }
                        case CryptoLibrary.BRWalletEventType.BITCOIN_WALLET_TRANSACTION_SUBMITTED: {
                            Log.d(TAG, String.format("BRWalletEventCallback: BITCOIN_WALLET_TRANSACTION_SUBMITTED (%s)", event.u.submitted.error));

                            Optional<Transfer> optTransfer = wallet.getOrCreateTransferByImpl(event.u.submitted.transaction, false);
                            if (optTransfer.isPresent()) {
                                announceWalletEvent(walletManager, wallet, new WalletTransferSubmittedEvent(optTransfer.get()));
                            }
                            break;
                        }
                        default:
                            throw new IllegalStateException(String.format("Unsupported BRWalletEventCallback type: %s", event.type));
                    }
                } else {
                    Log.d(TAG, String.format("BRWalletEventCallback: missed wallet for type: %s", event.type));
                }

            } else {
                Log.d(TAG, String.format("BRWalletEventCallback: missed manager for type: %s", event.type));
            }
        });
    }

    private void handleWalletManagerEvent(Pointer context, BRWalletManager managerImpl, BRWalletManagerEvent.ByValue event) {
        systemExecutor.submit(() -> {
            Optional<WalletManager> optManager = getWalletManagerByImpl(managerImpl);
            if (optManager.isPresent()) {
                WalletManager walletManager = optManager.get();

                switch (event.type) {
                    case CryptoLibrary.BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_CREATED: {
                        Log.d(TAG, "BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_CREATED");
                        announceWalletManagerEvent(walletManager, new WalletManagerCreatedEvent());
                        break;
                    }
                    case CryptoLibrary.BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_CONNECTED: {
                        Log.d(TAG, "BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_CONNECTED");

                        WalletManagerState newState = WalletManagerState.CONNECTED;
                        WalletManagerState oldState = walletManager.setState(newState);

                        announceWalletManagerEvent(walletManager, new WalletManagerChangedEvent(oldState, newState));
                        break;
                    }
                    case CryptoLibrary.BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_DISCONNECTED: {
                        Log.d(TAG, "BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_DISCONNECTED");

                        WalletManagerState newState = WalletManagerState.DISCONNECTED;
                        WalletManagerState oldState = walletManager.setState(newState);

                        announceWalletManagerEvent(walletManager, new WalletManagerChangedEvent(oldState, newState));
                        break;
                    }
                    case CryptoLibrary.BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_SYNC_STARTED: {
                        Log.d(TAG, "BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_SYNC_STARTED");

                        WalletManagerState newState = WalletManagerState.SYNCHING;
                        WalletManagerState oldState = walletManager.setState(newState);

                        announceWalletManagerEvent(walletManager, new WalletManagerSyncStartedEvent());
                        announceWalletManagerEvent(walletManager, new WalletManagerChangedEvent(oldState, newState));
                        break;
                    }
                    case CryptoLibrary.BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_SYNC_STOPPED: {
                        int error = event.u.syncStopped.error;
                        Log.d(TAG, String.format("BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_SYNC_STOPPED (%s)", error));

                        WalletManagerState newState = WalletManagerState.CONNECTED;
                        WalletManagerState oldState = walletManager.setState(newState);

                        announceWalletManagerEvent(walletManager, new WalletManagerSyncStoppedEvent(String.format("Code: %s", error)));
                        announceWalletManagerEvent(walletManager, new WalletManagerChangedEvent(oldState, newState));
                        break;
                    }
                    case CryptoLibrary.BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED: {
                        int height = event.u.blockHeightUpdated.value;
                        Log.d(TAG, String.format("BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED (%s)", height));

                        Network network = walletManager.getNetwork();
                        network.setHeight(height);

                        for (Wallet wallet: walletManager.getWallets()) {
                            for(Transfer transfer: wallet.getTransfers()) {
                                Optional<Long> optionalConfirmations = transfer.getConfirmationsAt(height);
                                if (optionalConfirmations.isPresent()) {
                                    long confirmations = optionalConfirmations.get();
                                    announceTransferEvent(walletManager, wallet, transfer, new TransferConfirmationEvent(confirmations));
                                }
                            }
                        }
                        break;
                    }
                    default:
                        throw new IllegalStateException(String.format("Unsupported BRWalletManagerEventCallback type: %s", event.type));
                }

            } else {
                Log.d(TAG, String.format("BRWalletManagerEventCallback: missed manager for type: %s", event.type));
            }

        });
    }

    // WalletManager management

    @Override
    public List<WalletManager> getWalletManagers() {
        return new ArrayList<>(walletManagers);
    }

    private void addWalletManager(WalletManager walletManager) {
        // TODO: This is from the swift code but I don't see how it would ever evaluate to true
        if (!walletManagers.contains(walletManager)) {
            walletManagers.add(walletManager);
            walletManager.initialize();

            announceSystemEvent(new SystemManagerAddedEvent(walletManager));
        }
    }

    private Optional<WalletManager> getWalletManagerByImpl(BRWalletManager manager) {
        for (WalletManager walletManager: walletManagers) {
            if (walletManager.matches(manager)) {
                return Optional.of(walletManager);
            }
        }
        return Optional.absent();
    }

    // Network management

    @Override
    public List<Network> getNetworks() {
        return new ArrayList<>(networks);
    }

    private void addNetwork(Network network) {
        // TODO: The swift code has no guards against a network being added multiple times; should it?
        if (!networks.contains(network)) {
            networks.add(network);

            announceNetworkEvent(network, new NetworkCreatedEvent());
            announceSystemEvent(new SystemNetworkAddedEvent(network));
        }
    }
}
