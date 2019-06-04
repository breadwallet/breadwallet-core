package com.breadwallet.crypto.implj;

import android.util.Log;

import com.breadwallet.crypto.Account;
import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Network;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferConfirmation;
import com.breadwallet.crypto.TransferState;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletManagerState;
import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.events.transfer.TransferChangedEvent;
import com.breadwallet.crypto.events.transfer.TransferConfirmationEvent;
import com.breadwallet.crypto.events.transfer.TransferCreatedEvent;
import com.breadwallet.crypto.events.transfer.TransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletBalanceUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferAddedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferSubmittedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerChangedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerCreatedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStartedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStoppedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletChangedEvent;
import com.breadwallet.crypto.libcrypto.bitcoin.BRPeerManager;
import com.breadwallet.crypto.libcrypto.bitcoin.BRPeerManager.BRPeerManagerPublishTxCallback;
import com.breadwallet.crypto.libcrypto.bitcoin.BRTransaction;
import com.breadwallet.crypto.libcrypto.bitcoin.BRTransactionEvent;
import com.breadwallet.crypto.libcrypto.bitcoin.BRTransactionEventType;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWalletEvent;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWalletEventType;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWalletManager;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWalletManagerEvent;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWalletManagerEventType;
import com.breadwallet.crypto.libcrypto.bitcoin.CoreBRTransaction;
import com.breadwallet.crypto.libcrypto.bitcoin.CoreBRWalletManager;
import com.breadwallet.crypto.libcrypto.support.BRMasterPubKey;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWalletManagerClient;
import com.breadwallet.crypto.libcrypto.support.BRSyncMode;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWallet;
import com.breadwallet.crypto.libcrypto.bitcoin.BRChainParams;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/* package */
final class WalletManagerImplBtc extends WalletManagerImpl<WalletImplBtc> {

    private static final String TAG = WalletManagerImplBtc.class.getName();

    private static final int UNUSED_ADDR_LIMIT = 25;

    private static int modeAsBtc(WalletManagerMode mode) {
        switch (mode) {
            case API_ONLY:
                return BRSyncMode.SYNC_MODE_BRD_ONLY;
            case P2P_ONLY:
                return BRSyncMode.SYNC_MODE_P2P_ONLY;
            case P2P_WITH_API_SYNC:
                return BRSyncMode.SYNC_MODE_P2P_WITH_BRD_SYNC;
            case API_WITH_P2P_SUBMIT:
                return BRSyncMode.SYNC_MODE_BRD_WITH_P2P_SEND;
            default:
                throw new IllegalStateException("Invalid mode");
        }
    }

    private final BRPeerManagerPublishTxCallback publishTxCallback = (info, error) -> {
        // TODO(fix): How do we want to propagating the error or success to the system listener? Or should we be using BRWalletManagerSubmitTransaction?
        // TODO(fix): If there is an error, do we need to call BRTransactionFree on the originating transaction? Or should we be using BRWalletManagerSubmitTransaction?
    };

    private final BRWalletManagerClient.ByValue clientCallbacks = new BRWalletManagerClient.ByValue(Pointer.NULL,
            this::doGetBlockNumber,
            this::doGetTransactions,
            this::doSubmitTransation,
            this::handleTransactionEvent,
            this::handleWalletEvent,
            this::handleWalletManagerEvent);

    private final ExecutorService systemExecutor;
    private final SystemAnnouncer announcer;
    private final BlockchainDb query;
    private final CoreBRWalletManager coreManager;

    /* package */
    WalletManagerImplBtc(Account account,
                         Network network,
                         WalletManagerMode managerMode,
                         String path,
                         SystemAnnouncer announcer,
                         BlockchainDb query) {
        super(account, network, managerMode, path);
        this.systemExecutor = Executors.newSingleThreadExecutor();
        this.announcer = announcer;
        this.query = query;

        BRMasterPubKey.ByValue pubKey = account.asBtc();
        BRChainParams chainParams = network.asBtc();
        int timestamp = (int) account.getTimestamp();
        int mode = modeAsBtc(managerMode);
        this.coreManager = CoreBRWalletManager.create(clientCallbacks, pubKey, chainParams, timestamp, mode, path);

        setPrimaryWallet(coreManager.getWallet());
    }

    @Override
    public void initialize() {
        // TODO(fix): Add call to CryptoLibrary.INSTANCE.BRWalletManagerInit(coreManager);
    }

    @Override
    public void connect() {
        coreManager.connect();
    }

    @Override
    public void disconnect() {
        coreManager.disconnect();
    }

    @Override
    public void sync() {
        coreManager.scan();
    }

    @Override
    public void submit(Transfer transfer, String paperKey) {
        if (transfer instanceof TransferImplBtc) {
            BRWallet coreWallet = coreManager.getWallet();
            BRPeerManager corePeerManager = coreManager.getPeerManager();
            CoreBRTransaction coreTransaction = ((TransferImplBtc) transfer).getCoreBRTransaction();

            byte[] seed = AccountImpl.deriveSeed(paperKey);

            // TODO(discuss): How do we want to handle signing failure? Or should we be using BRWalletManagerSubmitTransaction?
            boolean signed = coreWallet.signTransaction(coreTransaction, seed);

            corePeerManager.publishTransaction(coreTransaction, publishTxCallback);
        } else {
            throw new IllegalArgumentException("Unsupported transfer type");
        }
    }

    private Optional<WalletImplBtc> getOrCreateWalletByImpl(BRWallet walletImpl, boolean createAllowed) {
        walletsWriteLock.lock();
        try {
            Optional<WalletImplBtc> optWallet = getWalletByImplUnderLock(walletImpl);
            if (optWallet.isPresent()) {
                return optWallet;
            } else if (createAllowed) {
                return Optional.of(addWalletByImplUnderLock(walletImpl));
            } else {
                return Optional.absent();
            }
        } finally {
            walletsWriteLock.unlock();
        }
    }

    private void setPrimaryWallet(BRWallet walletImpl) {
        walletsWriteLock.lock();
        try {
            Optional<WalletImplBtc> optWallet = getWalletByImplUnderLock(walletImpl);
            if (!optWallet.isPresent()) {
                wallets.add(0, new WalletImplBtc(this, walletImpl, getBaseUnit(), getDefaultUnit()));
            }
        } finally {
            walletsWriteLock.unlock();
        }
    }

    private WalletImplBtc addWalletByImplUnderLock(BRWallet walletImpl) {
        WalletImplBtc wallet = new WalletImplBtc(this, walletImpl, getBaseUnit(), getDefaultUnit());
        wallets.add(wallet);
        return wallet;
    }

    private Optional<WalletImplBtc> getWalletByImplUnderLock(BRWallet walletImpl) {
        for (WalletImplBtc wallet : wallets) {
            if (wallet.matches(walletImpl)) {
                return Optional.of(wallet);
            }
        }
        return Optional.absent();
    }

    private
    List<String> getAllAddrs() {
        coreManager.generateUnusedAddrs(UNUSED_ADDR_LIMIT);
        List<String> addrs = new ArrayList<>(coreManager.getAllAddrs());
        addrs.addAll(coreManager.getAllAddrsLegacy());
        return addrs;
    }

    private
    void announceBlockNumber(int rid, long blockNumber) {
        coreManager.announceBlockNumber(rid, blockNumber);
    }

    private
    void announceSubmit(int rid, CoreBRTransaction transaction, int i) {
        coreManager.announceSubmit(rid, transaction, i);
    }

    private
    void announceTransaction(int rid, CoreBRTransaction transaction) {
        coreManager.announceTransaction(rid, transaction);
    }

    private
    void announceTransactionComplete(int rid, boolean success) {
        coreManager.announceTransactionComplete(rid, success);
    }

    // BRWalletManagerClient Callbacks

    private void doGetBlockNumber(Pointer context, BRWalletManager managerImpl, int rid) {
        systemExecutor.submit(() -> {
            Log.d(TAG, "BRGetBlockNumberCallback");
            query.getBlockchain(getNetwork().getUids(), new BlockchainCompletionHandler<Blockchain>() {
                @Override
                public void handleData(Blockchain blockchain) {
                    long blockchainHeight = blockchain.getBlockHeight();
                    Log.d(TAG, String.format("BRGetBlockNumberCallback: succeeded with block number %s", blockchainHeight));
                    announceBlockNumber(rid, blockchainHeight);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.d(TAG, "BRGetBlockNumberCallback: failed", error);
                }
            });
        });
    }

    private void doGetTransactions(Pointer context, BRWalletManager managerImpl, long begBlockNumber, long endBlockNumber, int rid) {
        systemExecutor.submit(() -> {
            Log.d(TAG, String.format("BRGetTransactionsCallback (%s %s)", begBlockNumber, endBlockNumber));

            String blockchainId = getNetwork().getUids();
            List<String> addresses = getAllAddrs();
            Set<String> knownAddresses = new HashSet<>(addresses);

            query.getTransactions(blockchainId, addresses, begBlockNumber, endBlockNumber, true, false, new BlockchainCompletionHandler<List<Transaction>>() {
                @Override
                public void handleData(List<Transaction> transactions) {
                    Log.d(TAG, "BRGetTransactionsCallback: transaction success");

                    for (Transaction transaction: transactions) {
                        Optional<byte[]> optRaw = transaction.getRaw();
                        if (!optRaw.isPresent()) {
                            announceTransactionComplete(rid, false);
                            return;
                        }

                        long timestamp = transaction.getTimestamp().transform((ts) -> ts.getTime() / 1000).or(0L);
                        if (timestamp < 0 || timestamp > Integer.MAX_VALUE) {
                            Log.d(TAG, "BRGetTransactionsCallback received invalid timestamp, completing with failure");
                            announceTransactionComplete(rid, false);
                            return;
                        }

                        long blockHeight = transaction.getBlockHeight().or(0L);
                        if (blockHeight < 0 || blockHeight > Integer.MAX_VALUE) {
                            Log.d(TAG, "BRGetTransactionsCallback received invalid block height, completing with failure");
                            announceTransactionComplete(rid, false);
                            return;
                        }

                        // TODO (fix): Are these casts safe?
                        Optional<CoreBRTransaction> optCore = CoreBRTransaction.create(optRaw.get(), (int) timestamp, (int) blockHeight);
                        if (!optCore.isPresent()) {
                            Log.d(TAG, "BRGetTransactionsCallback received invalid transaction, completing with failure");
                            announceTransactionComplete(rid, false);
                            return;
                        }

                        Log.d(TAG, "BRGetTransactionsCallback received transaction, announcing " + transaction.getId());
                        announceTransaction(rid, optCore.get());
                    }

                    if (transactions.isEmpty()) {
                        Log.d(TAG, "BRGetTransactionsCallback found no transactions, completing");
                        announceTransactionComplete(rid, true);
                    } else {
                        Set<String> addressesSet = new HashSet<>(getAllAddrs());
                        addressesSet.removeAll(knownAddresses);
                        knownAddresses.addAll(addressesSet);
                        List<String> addresses = new ArrayList<>(addressesSet);

                        Log.d(TAG, "BRGetTransactionsCallback found transactions, requesting again");
                        systemExecutor.submit(() -> query.getTransactions(blockchainId, addresses, begBlockNumber, endBlockNumber, true, false, this));
                    }
                }

                @Override
                public void handleError(QueryError error) {
                    Log.d(TAG, "BRGetTransactionsCallback received an error, completing with failure", error);
                    announceTransactionComplete(rid, false);
                }
            });
        });
    }

    private void doSubmitTransation(Pointer context, BRWalletManager managerImpl, BRWallet walletImpl, BRTransaction transactionImpl, int rid) {
        systemExecutor.submit(() -> {
            Optional<WalletImplBtc> optWallet = getOrCreateWalletByImpl(walletImpl, false);
            if (optWallet.isPresent()) {
                WalletImplBtc wallet = optWallet.get();

                Optional<TransferImplBtc> optTransfer = wallet.getOrCreateTransferByImpl(transactionImpl, false);
                if (optTransfer.isPresent()) {
                    TransferImplBtc transfer = optTransfer.get();

                    Log.d(TAG, "BRSubmitTransactionCallback");
                    query.putTransaction(getNetwork().getUids(), transfer.serialize(), new BlockchainCompletionHandler<Transaction>() {
                        @Override
                        public void handleData(Transaction data) {
                            Log.d(TAG, "BRSubmitTransactionCallback: succeeded");
                            announceSubmit(rid, transfer.getCoreBRTransaction(), 0);
                        }

                        @Override
                        public void handleError(QueryError error) {
                            Log.d(TAG, "BRSubmitTransactionCallback: failed", error);
                            announceSubmit(rid, transfer.getCoreBRTransaction(), 1);
                        }
                    });

                } else {
                    Log.d(TAG, "BRSubmitTransactionCallback: missed transfer");
                }

            } else {
                Log.d(TAG, "BRSubmitTransactionCallback: missed wallet");
            }
        });
    }

    private void handleTransactionEvent(Pointer context, BRWalletManager managerImpl, BRWallet walletImpl, BRTransaction transactionImpl, BRTransactionEvent.ByValue event) {
        systemExecutor.submit(() -> {
            Optional<WalletImplBtc> optWallet = getOrCreateWalletByImpl(walletImpl, false);
            if (optWallet.isPresent()) {
                WalletImplBtc wallet = optWallet.get();

                boolean createAllowed = event.type == BRTransactionEventType.BITCOIN_TRANSACTION_ADDED;
                Optional<TransferImplBtc> optTransfer = wallet.getOrCreateTransferByImpl(transactionImpl, createAllowed);
                if (optTransfer.isPresent()) {
                    TransferImpl transfer = optTransfer.get();

                    switch (event.type) {
                        case BRTransactionEventType.BITCOIN_TRANSACTION_ADDED:
                            Log.d(TAG, "BRTransactionEventCallback: BITCOIN_TRANSACTION_ADDED");

                            announcer.announceTransferEvent(this, wallet, transfer, new TransferCreatedEvent());
                            announcer.announceWalletEvent(this, wallet, new WalletTransferAddedEvent(transfer));
                            break;
                        case BRTransactionEventType.BITCOIN_TRANSACTION_DELETED:
                            Log.d(TAG, "BRTransactionEventCallback: BITCOIN_TRANSACTION_DELETED");

                            announcer.announceTransferEvent(this, wallet, transfer, new TransferDeletedEvent());
                            announcer.announceWalletEvent(this, wallet, new WalletTransferDeletedEvent(transfer));
                            break;
                        case BRTransactionEventType.BITCOIN_TRANSACTION_UPDATED:
                            int blockHeight = event.u.updated.blockHeight;
                            int timestamp = event.u.updated.timestamp;
                            Log.d(TAG, String.format("BRTransactionEventCallback: BITCOIN_TRANSACTION_UPDATED (%s, %s)", blockHeight, timestamp));

                            Optional<Amount> optionalAmount = AmountImpl.create(0, getDefaultUnit());
                            if (optionalAmount.isPresent()) {
                                TransferConfirmation confirmation = new TransferConfirmation(blockHeight, 0, timestamp, optionalAmount.get());

                                TransferState newState = TransferState.INCLUDED(confirmation);
                                TransferState oldState = transfer.setState(newState);

                                announcer.announceTransferEvent(this, wallet, transfer, new TransferChangedEvent(oldState, newState));
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
        });
    }

    private void handleWalletEvent(Pointer context, BRWalletManager managerImpl, BRWallet walletImpl, BRWalletEvent.ByValue event) {
        systemExecutor.submit(() -> {
            boolean createAllowed = event.type == BRWalletEventType.BITCOIN_WALLET_CREATED;
            Optional<WalletImplBtc> optWallet = getOrCreateWalletByImpl(walletImpl, createAllowed);
            if (optWallet.isPresent()) {
                WalletImplBtc wallet = optWallet.get();

                switch (event.type) {
                    case BRWalletEventType.BITCOIN_WALLET_CREATED: {
                        Log.d(TAG, "BRWalletEventCallback: BITCOIN_WALLET_CREATED");

                        announcer.announceWalletEvent(this, wallet, new WalletCreatedEvent());
                        break;
                    }
                    case BRWalletEventType.BITCOIN_WALLET_DELETED: {
                        Log.d(TAG, "BRWalletEventCallback: BITCOIN_WALLET_DELETED");

                        announcer.announceWalletEvent(this, wallet, new WalletDeletedEvent());
                        break;
                    }
                    case BRWalletEventType.BITCOIN_WALLET_BALANCE_UPDATED: {
                        Log.d(TAG, String.format("BRWalletEventCallback: BITCOIN_WALLET_BALANCE_UPDATED (%s)", event.u.balance.satoshi));

                        announcer.announceWalletEvent(this, wallet, new WalletBalanceUpdatedEvent(wallet.getBalance()));
                        announcer.announceWalletManagerEvent(this, new WalletManagerWalletChangedEvent(wallet));
                        break;
                    }
                    case BRWalletEventType.BITCOIN_WALLET_TRANSACTION_SUBMITTED: {
                        Log.d(TAG, String.format("BRWalletEventCallback: BITCOIN_WALLET_TRANSACTION_SUBMITTED (%s)", event.u.submitted.error));

                        Optional<TransferImplBtc> optTransfer = wallet.getOrCreateTransferByImpl(event.u.submitted.transaction, false);
                        if (optTransfer.isPresent()) {
                            announcer.announceWalletEvent(this, wallet, new WalletTransferSubmittedEvent(optTransfer.get()));
                        }
                        break;
                    }
                    default:
                        throw new IllegalStateException(String.format("Unsupported BRWalletEventCallback type: %s", event.type));
                }
            } else {
                Log.d(TAG, String.format("BRWalletEventCallback: missed wallet for type: %s", event.type));
            }
        });
    }

    private void handleWalletManagerEvent(Pointer context, BRWalletManager managerImpl, BRWalletManagerEvent.ByValue event) {
        systemExecutor.submit(() -> {
            switch (event.type) {
                case BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_CREATED: {
                    Log.d(TAG, "BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_CREATED");
                    announcer.announceWalletManagerEvent(this, new WalletManagerCreatedEvent());
                    break;
                }
                case BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_CONNECTED: {
                    Log.d(TAG, "BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_CONNECTED");

                    WalletManagerState newState = WalletManagerState.CONNECTED;
                    WalletManagerState oldState = setState(newState);

                    announcer.announceWalletManagerEvent(this, new WalletManagerChangedEvent(oldState, newState));
                    break;
                }
                case BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_DISCONNECTED: {
                    Log.d(TAG, "BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_DISCONNECTED");

                    WalletManagerState newState = WalletManagerState.DISCONNECTED;
                    WalletManagerState oldState = setState(newState);

                    announcer.announceWalletManagerEvent(this, new WalletManagerChangedEvent(oldState, newState));
                    break;
                }
                case BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_SYNC_STARTED: {
                    Log.d(TAG, "BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_SYNC_STARTED");

                    WalletManagerState newState = WalletManagerState.SYNCING;
                    WalletManagerState oldState = setState(newState);

                    announcer.announceWalletManagerEvent(this, new WalletManagerSyncStartedEvent());
                    announcer.announceWalletManagerEvent(this, new WalletManagerChangedEvent(oldState, newState));
                    break;
                }
                case BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_SYNC_STOPPED: {
                    int error = event.u.syncStopped.error;
                    Log.d(TAG, String.format("BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_SYNC_STOPPED (%s)", error));

                    WalletManagerState newState = WalletManagerState.CONNECTED;
                    WalletManagerState oldState = setState(newState);

                    announcer.announceWalletManagerEvent(this, new WalletManagerSyncStoppedEvent(String.format("Code: %s", error)));
                    announcer.announceWalletManagerEvent(this, new WalletManagerChangedEvent(oldState, newState));
                    break;
                }
                case BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED: {
                    int height = event.u.blockHeightUpdated.value;
                    Log.d(TAG, String.format("BRWalletManagerEventCallback: BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED (%s)", height));

                    Network network = getNetwork();
                    network.setHeight(height);

                    for (Wallet wallet: getWallets()) {
                        for(Transfer transfer: wallet.getTransfers()) {
                            Optional<Long> optionalConfirmations = transfer.getConfirmationsAt(height);
                            if (optionalConfirmations.isPresent()) {
                                long confirmations = optionalConfirmations.get();
                                announcer.announceTransferEvent(this, wallet, transfer, new TransferConfirmationEvent(confirmations));
                            }
                        }
                    }
                    break;
                }
                default:
                    throw new IllegalStateException(String.format("Unsupported BRWalletManagerEventCallback type: %s", event.type));
            }
        });
    }
}
