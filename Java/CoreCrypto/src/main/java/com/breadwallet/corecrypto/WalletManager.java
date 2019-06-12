package com.breadwallet.corecrypto;

import android.util.Log;

import com.breadwallet.corenative.bitcoin.BRTransaction;
import com.breadwallet.corenative.bitcoin.BRWallet;
import com.breadwallet.corenative.bitcoin.BRWalletManager;
import com.breadwallet.corenative.bitcoin.CoreBRTransaction;
import com.breadwallet.corenative.crypto.BRCryptoCWMClient;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientBtc;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientEth;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener;
import com.breadwallet.corenative.crypto.BRCryptoTransfer;
import com.breadwallet.corenative.crypto.BRCryptoTransferEvent;
import com.breadwallet.corenative.crypto.BRCryptoTransferEventType;
import com.breadwallet.corenative.crypto.BRCryptoWallet;
import com.breadwallet.corenative.crypto.BRCryptoWalletEvent;
import com.breadwallet.corenative.crypto.BRCryptoWalletEventType;
import com.breadwallet.corenative.crypto.BRCryptoWalletManager;
import com.breadwallet.corenative.crypto.BRCryptoWalletManagerEvent;
import com.breadwallet.corenative.crypto.BRCryptoWalletManagerEventType;
import com.breadwallet.corenative.crypto.CoreBRCryptoWallet;
import com.breadwallet.corenative.crypto.CoreBRCryptoWalletManager;
import com.breadwallet.crypto.TransferState;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletManagerState;
import com.breadwallet.crypto.WalletState;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.CompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.events.transfer.TransferChangedEvent;
import com.breadwallet.crypto.events.transfer.TransferConfirmationEvent;
import com.breadwallet.crypto.events.transfer.TransferCreatedEvent;
import com.breadwallet.crypto.events.transfer.TransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletBalanceUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletCreatedEvent;
import com.breadwallet.crypto.events.wallet.WalletDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletFeeBasisUpdatedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferAddedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferChangedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferDeletedEvent;
import com.breadwallet.crypto.events.wallet.WalletTransferSubmittedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerBlockUpdatedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerChangedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerCreatedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerDeletedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncProgressEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStartedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerSyncStoppedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletAddedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletChangedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerWalletDeletedEvent;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;

import java.util.ArrayList;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

/* package */
final class WalletManager implements com.breadwallet.crypto.WalletManager {

    private static final String TAG = WalletManager.class.getName();

    /* package */
    static WalletManager create(Account account, Network network, WalletManagerMode mode, String path,
                                SystemAnnouncer announcer, BlockchainDb query) {
        return new WalletManager(account, network, mode, path, announcer, query);
    }

    private final Account account;
    private final Network network;
    private final Currency networkCurrency;
    private final Unit networkBaseUnit;
    private final Unit networkDefaultUnit;
    private final String path;
    private final WalletManagerMode mode;

    private final ExecutorService systemExecutor;
    private final SystemAnnouncer announcer;
    private final BlockchainDb query;
    private final CoreBRCryptoWalletManager core;

    private final BRCryptoCWMListener.ByValue listener;
    private final BRCryptoCWMClient.ByValue client;

    private WalletManager(Account account,
                          Network network,
                          WalletManagerMode mode,
                          String path,
                          SystemAnnouncer announcer,
                          BlockchainDb query) {
        this.account = account;
        this.network = network;
        this.networkCurrency = network.getCurrency();
        this.mode = mode;
        this.path = path;

        // TODO(fix): This is how the swift behaves; not good!
        this.networkBaseUnit = network.baseUnitFor(networkCurrency).get();
        this.networkDefaultUnit = network.defaultUnitFor(networkCurrency).get();

        this.systemExecutor = Executors.newSingleThreadExecutor();
        this.announcer = announcer;
        this.query = query;

        this.listener = new BRCryptoCWMListener.ByValue(Pointer.NULL,
                this::walletManagerEventCallback,
                this::walletEventCallback,
                this::transferEventCallback);

        this.client = new BRCryptoCWMClient.ByValue(Pointer.NULL,
                new BRCryptoCWMClientBtc(
                        this::btcGetBlockNumber,
                        this::btcGetTransactions,
                        this::btcSubmitTransaction),
                new BRCryptoCWMClientEth(
                        this::ethGetBalance,
                        this::ethGetGasPrice,
                        this::ethEstimateGas,
                        this::ethSubmitTransaction,
                        this::ethGetTransactions,
                        this::ethGetLogs,
                        this::ethGetBlocks,
                        this::ethGetTokens,
                        this::ethGetBlockNumber,
                        this::ethGetNonce));

        this.core = CoreBRCryptoWalletManager.create(listener, client, account.getCoreBRCryptoAccount(),
                network.getCoreBRCryptoNetwork(), Utilities.walletManagerModeToCrypto(mode), path);
    }

    @Override
    public void connect() {
        core.connect();
    }

    @Override
    public void disconnect() {
        core.disconnect();
    }

    @Override
    public void sync() {
        core.sync();
    }

    @Override
    public void submit(com.breadwallet.crypto.Transfer transfer, String paperKey) {
        Transfer transferImpl = Transfer.from(transfer);
        Wallet walletImpl = transferImpl.getWallet();
        core.submit(walletImpl.getCoreBRCryptoWallet(), transferImpl.getCoreBRCryptoTransfer(), paperKey);
    }

    @Override
    public boolean isActive() {
        WalletManagerState state = getState();
        return state == WalletManagerState.CREATED || state == WalletManagerState.SYNCING;
    }

    @Override
    public Account getAccount() {
        return account;
    }

    @Override
    public Network getNetwork() {
        return network;
    }

    @Override
    public Wallet getPrimaryWallet() {
        return Wallet.create(core.getWallet(), this, networkBaseUnit, networkDefaultUnit);
    }

    @Override
    public List<Wallet> getWallets() {
        List<Wallet> wallets = new ArrayList<>();

        UnsignedLong count = core.getWalletsCount();
        for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
            wallets.add(Wallet.create(core.getWallet(i), this, networkBaseUnit, networkDefaultUnit));
        }

        return wallets;
    }

    @Override
    public WalletManagerMode getMode() {
        return mode;
    }

    @Override
    public String getPath() {
        return path;
    }

    @Override
    public Currency getCurrency() {
        return networkCurrency;
    }

    @Override
    public String getName() {
        return networkCurrency.getCode();
    }

    @Override
    public Unit getBaseUnit() {
        return networkBaseUnit;
    }

    @Override
    public Unit getDefaultUnit() {
        return networkDefaultUnit;
    }

    @Override
    public WalletManagerState getState() {
        return Utilities.walletManagerStateFromCrypto(core.getState());
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof WalletManager)) {
            return false;
        }

        WalletManager that = (WalletManager) object;
        return core.equals(that.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }

    private Optional<Wallet> getWallet(CoreBRCryptoWallet value) {
        UnsignedLong count = core.getWalletsCount();
        for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
            if (core.getWallet(i).equals(value)) {
                return Optional.of(Wallet.create(core.getWallet(i), this, networkBaseUnit, networkDefaultUnit));
            }
        }
        return Optional.absent();
    }

    private void walletManagerEventCallback(Pointer context, BRCryptoWalletManager manager,
                                            BRCryptoWalletManagerEvent.ByValue event) {
        systemExecutor.submit(() -> {
            switch (event.type) {
                case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_CREATED: {
                    Log.d(TAG, "CRYPTO_WALLET_MANAGER_EVENT_CREATED");
                    announcer.announceWalletManagerEvent(this, new WalletManagerCreatedEvent());
                    // TODO(fix): This is a hack, remove!
                    announcer.announceWalletEvent(this, getPrimaryWallet(), new WalletCreatedEvent());
                    break;
                }
                case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_CHANGED: {
                    WalletManagerState oldState = Utilities.walletManagerStateFromCrypto(event.u.state.oldValue);
                    WalletManagerState newState = Utilities.walletManagerStateFromCrypto(event.u.state.newValue);
                    Log.d(TAG, String.format("CRYPTO_WALLET_MANAGER_EVENT_CHANGED (%s -> %s)", oldState, newState));
                    announcer.announceWalletManagerEvent(this, new WalletManagerChangedEvent(
                            oldState, newState));
                    break;
                }
                case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_DELETED: {
                    Log.d(TAG, "CRYPTO_WALLET_MANAGER_EVENT_DELETED");
                    announcer.announceWalletManagerEvent(this, new WalletManagerDeletedEvent());
                    break;
                }
                case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED: {
                    Log.d(TAG, "CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED");
                    Optional<Wallet> optional = getWallet(event.u.wallet.value);
                    if (optional.isPresent()) {
                        Wallet wallet = optional.get();
                        announcer.announceWalletManagerEvent(this, new WalletManagerWalletAddedEvent(wallet));
                    } else {
                        Log.e(TAG, "CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED: missed wallet");
                    }
                    break;
                }
                case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED: {
                    Log.d(TAG, "CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED");
                    Optional<Wallet> optional = getWallet(event.u.wallet.value);
                    if (optional.isPresent()) {
                        Wallet wallet = optional.get();
                        announcer.announceWalletManagerEvent(this, new WalletManagerWalletChangedEvent(wallet));
                    } else {
                        Log.e(TAG, "CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED: missed wallet");
                    }
                    break;
                }
                case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED: {
                    Log.d(TAG, "CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED");
                    Optional<Wallet> optional = getWallet(event.u.wallet.value);
                    if (optional.isPresent()) {
                        Wallet wallet = optional.get();
                        announcer.announceWalletManagerEvent(this, new WalletManagerWalletDeletedEvent(wallet));
                    } else {
                        Log.e(TAG, "CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED: missed wallet");
                    }
                    break;
                }
                case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED: {
                    Log.d(TAG, "CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED");
                    announcer.announceWalletManagerEvent(this, new WalletManagerSyncStartedEvent());
                    break;
                }
                case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES: {
                    double percent = event.u.sync.percentComplete;
                    Log.d(TAG, String.format("CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES (%s)", percent));
                    announcer.announceWalletManagerEvent(this,
                            new WalletManagerSyncProgressEvent(percent));
                    break;
                }
                case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED: {
                    Log.d(TAG, "CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED");
                    // TODO(fix): fill in message
                    announcer.announceWalletManagerEvent(this, new WalletManagerSyncStoppedEvent(""));
                    break;
                }
                case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED: {
                    UnsignedLong blockHeight = UnsignedLong.fromLongBits(event.u.blockHeight.value);
                    Log.d(TAG, String.format("CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED (%s)", blockHeight));
                    announcer.announceWalletManagerEvent(this, new WalletManagerBlockUpdatedEvent(blockHeight));

                    network.setHeight(blockHeight);
                    for (Wallet wallet : getWallets()) {
                        for (Transfer transfer : wallet.getTransfers()) {
                            Optional<UnsignedLong> optionalConfirmations = transfer.getConfirmationsAt(blockHeight);
                            if (optionalConfirmations.isPresent()) {
                                UnsignedLong confirmations = optionalConfirmations.get();
                                announcer.announceTransferEvent(this, wallet, transfer,
                                        new TransferConfirmationEvent(confirmations));
                            }
                        }
                    }
                    break;
                }
            }
        });
    }

    private void walletEventCallback(Pointer context, BRCryptoWalletManager manager, BRCryptoWallet wallet
            , BRCryptoWalletEvent.ByValue event) {
        systemExecutor.submit(() -> {
            Optional<Wallet> optWallet = getWallet(wallet);
            if (optWallet.isPresent()) {
                Wallet walletImpl = optWallet.get();

                switch (event.type) {
                    case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_CREATED: {
                        Log.d(TAG, "CRYPTO_WALLET_EVENT_CREATED");
                        announcer.announceWalletEvent(this, walletImpl, new WalletCreatedEvent());
                        break;
                    }
                    case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_CHANGED: {
                        WalletState oldState = Utilities.walletStateFromCrypto(event.u.state.oldState);
                        WalletState newState = Utilities.walletStateFromCrypto(event.u.state.newState);
                        Log.d(TAG, String.format("CRYPTO_WALLET_EVENT_CHANGED (%s -> %s)", oldState, newState));
                        announcer.announceWalletEvent(this, walletImpl, new WalletChangedEvent(
                                oldState, newState));
                        break;
                    }
                    case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_DELETED: {
                        Log.d(TAG, "CRYPTO_WALLET_EVENT_DELETED");
                        announcer.announceWalletEvent(this, walletImpl, new WalletDeletedEvent());
                        break;
                    }
                    case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_ADDED: {
                        Log.d(TAG, "CRYPTO_WALLET_EVENT_TRANSFER_ADDED");
                        Optional<Transfer> optional = walletImpl.getTransfer(event.u.transfer.value);
                        if (optional.isPresent()) {
                            Transfer transfer = optional.get();
                            announcer.announceWalletEvent(this, walletImpl,
                                    new WalletTransferAddedEvent(transfer));
                        } else {
                            Log.e(TAG, "CRYPTO_WALLET_EVENT_TRANSFER_ADDED: missed transfer");
                        }
                        break;
                    }
                    case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_CHANGED: {
                        Log.d(TAG, "CRYPTO_WALLET_EVENT_TRANSFER_CHANGED");
                        Optional<Transfer> optional = walletImpl.getTransfer(event.u.transfer.value);
                        if (optional.isPresent()) {
                            Transfer transfer = optional.get();
                            announcer.announceWalletEvent(this, walletImpl,
                                    new WalletTransferChangedEvent(transfer));
                        } else {
                            Log.e(TAG, "CRYPTO_WALLET_EVENT_TRANSFER_CHANGED: missed transfer");
                        }
                        break;
                    }
                    case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED: {
                        Log.d(TAG, "CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED");
                        Optional<Transfer> optional = walletImpl.getTransfer(event.u.transfer.value);
                        if (optional.isPresent()) {
                            Transfer transfer = optional.get();
                            announcer.announceWalletEvent(this, walletImpl,
                                    new WalletTransferSubmittedEvent(transfer));
                        } else {
                            Log.e(TAG, "CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED: missed transfer");
                        }
                        break;
                    }
                    case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_DELETED: {
                        Log.d(TAG, "CRYPTO_WALLET_EVENT_TRANSFER_DELETED");
                        Optional<Transfer> optional = walletImpl.getTransfer(event.u.transfer.value);
                        if (optional.isPresent()) {
                            Transfer transfer = optional.get();
                            announcer.announceWalletEvent(this, walletImpl,
                                    new WalletTransferDeletedEvent(transfer));
                        } else {
                            Log.e(TAG, "CRYPTO_WALLET_EVENT_TRANSFER_DELETED: missed transfer");
                        }
                        break;
                    }
                    case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_BALANCE_UPDATED: {
                        Amount balance = walletImpl.getBalance();
                        Log.d(TAG, String.format("CRYPTO_WALLET_EVENT_BALANCE_UPDATED (%s)", balance));
                        // TODO(discuss): Should the amount come from the event?
                        announcer.announceWalletEvent(this, walletImpl,
                                new WalletBalanceUpdatedEvent(balance));
                        break;
                    }
                    case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED: {
                        TransferFeeBasis feeBasis = walletImpl.getDefaultFeeBasis();
                        Log.d(TAG, String.format("CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED (%s)", feeBasis));
                        // TODO(discuss): Should the fee basis come from the event?
                        announcer.announceWalletEvent(this, walletImpl,
                                new WalletFeeBasisUpdatedEvent(feeBasis));
                        break;
                    }
                }
            } else {
                Log.e(TAG, "walletEventCallback: missed wallet");
            }
        });
    }

    private void transferEventCallback(Pointer context, BRCryptoWalletManager manager,
                                       BRCryptoWallet wallet, BRCryptoTransfer transfer,
                                       BRCryptoTransferEvent.ByValue event) {
        systemExecutor.submit(() -> {
            Optional<Wallet> optWallet = getWallet(wallet);
            if (optWallet.isPresent()) {
                Wallet walletImpl = optWallet.get();

                Optional<Transfer> optTransfer = walletImpl.getTransfer(transfer);
                if (optTransfer.isPresent()) {
                    Transfer transferImpl = optTransfer.get();

                    switch (event.type) {
                        case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_CREATED: {
                            Log.d(TAG, "CRYPTO_TRANSFER_EVENT_CREATED");
                            announcer.announceTransferEvent(this, walletImpl, transferImpl, new TransferCreatedEvent());
                            break;
                        }
                        case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_CHANGED: {
                            TransferState oldState = Utilities.transferStateFromCrypto(event.u.state.oldState);
                            TransferState newState = Utilities.transferStateFromCrypto(event.u.state.newState);
                            Log.d(TAG, String.format("CRYPTO_TRANSFER_EVENT_CHANGED (%s -> %s)"));
                            announcer.announceTransferEvent(this, walletImpl, transferImpl, new TransferChangedEvent(
                                    oldState, newState));
                            break;
                        }
                        case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_CONFIRMED: {
                            UnsignedLong confirmations = UnsignedLong.fromLongBits(event.u.confirmation.count);
                            Log.d(TAG, String.format("CRYPTO_TRANSFER_EVENT_CONFIRMED (%s)", confirmations));
                            announcer.announceTransferEvent(this, walletImpl, transferImpl,
                                    new TransferConfirmationEvent(confirmations));
                            break;
                        }
                        case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_DELETED: {
                            Log.d(TAG, "CRYPTO_TRANSFER_EVENT_DELETED");
                            announcer.announceTransferEvent(this, walletImpl, transferImpl, new TransferDeletedEvent());
                            break;
                        }
                    }

                } else {
                    Log.e(TAG, "transferEventCallback: missed transfer");
                }
            } else {
                Log.e(TAG, "transferEventCallback: missed wallet");
            }
        });
    }

    // BTC client

    private void btcGetBlockNumber(Pointer context, BRWalletManager managerImpl, int rid) {
        systemExecutor.submit(() -> {
            Log.d(TAG, "BRGetBlockNumberCallback");
            query.getBlockchain(getNetwork().getUids(), new CompletionHandler<Blockchain>() {
                @Override
                public void handleData(Blockchain blockchain) {
                    UnsignedLong blockchainHeight = blockchain.getBlockHeight();
                    Log.d(TAG, String.format("BRGetBlockNumberCallback: succeeded (%s)",
                            blockchainHeight));
                    managerImpl.announceBlockNumber(rid, blockchainHeight);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.d(TAG, "BRGetBlockNumberCallback: failed", error);
                }
            });
        });
    }

    private void btcGetTransactions(Pointer context, BRWalletManager managerImpl, long begBlockNumber,
                                    long endBlockNumber, int rid) {
        systemExecutor.submit(() -> {
            UnsignedLong begBlockNumberUnsigned = UnsignedLong.fromLongBits(begBlockNumber);
            UnsignedLong endBlockNumberUnsigned = UnsignedLong.fromLongBits(endBlockNumber);

            Log.d(TAG, String.format("BRGetTransactionsCallback (%s -> %s)", begBlockNumberUnsigned,
                    endBlockNumberUnsigned));

            String blockchainId = getNetwork().getUids();
            List<String> addresses = getAllAddrs(managerImpl);
            Set<String> knownAddresses = new HashSet<>(addresses);

            query.getTransactions(blockchainId, addresses, begBlockNumberUnsigned, endBlockNumberUnsigned, true,
                    false, new CompletionHandler<List<Transaction>>() {
                        @Override
                        public void handleData(List<Transaction> transactions) {
                            Log.d(TAG, "BRGetTransactionsCallback: transaction success");

                            for (Transaction transaction : transactions) {
                                Optional<byte[]> optRaw = transaction.getRaw();
                                if (!optRaw.isPresent()) {
                                    managerImpl.announceTransactionComplete(rid, false);
                                    return;
                                }

                                int blockHeight =
                                        UnsignedInts.checkedCast(transaction.getBlockHeight().or(UnsignedLong.ZERO).longValue());
                                int timestamp =
                                        UnsignedInts.checkedCast(transaction.getTimestamp().transform(Date::getTime).transform(TimeUnit.MILLISECONDS::toSeconds).or(0L));

                                Optional<CoreBRTransaction> optCore = CoreBRTransaction.create(optRaw.get(), timestamp,
                                        blockHeight);
                                if (!optCore.isPresent()) {
                                    Log.d(TAG, "BRGetTransactionsCallback received invalid transaction, completing " +
                                            "with " +
                                            "failure");
                                    managerImpl.announceTransactionComplete(rid, false);
                                    return;
                                }

                                Log.d(TAG,
                                        "BRGetTransactionsCallback received transaction, announcing " + transaction.getId());
                                managerImpl.announceTransaction(rid, optCore.get());
                            }

                            if (transactions.isEmpty()) {
                                Log.d(TAG, "BRGetTransactionsCallback found no transactions, completing");
                                managerImpl.announceTransactionComplete(rid, true);
                            } else {
                                Set<String> addressesSet = new HashSet<>(getAllAddrs(managerImpl));
                                addressesSet.removeAll(knownAddresses);
                                knownAddresses.addAll(addressesSet);
                                List<String> addresses = new ArrayList<>(addressesSet);

                                Log.d(TAG, "BRGetTransactionsCallback found transactions, requesting again");
                                systemExecutor.submit(() -> query.getTransactions(blockchainId, addresses,
                                        begBlockNumberUnsigned, endBlockNumberUnsigned, true, false, this));
                            }
                        }

                        @Override
                        public void handleError(QueryError error) {
                            Log.d(TAG, "BRGetTransactionsCallback received an error, completing with failure", error);
                            managerImpl.announceTransactionComplete(rid, false);
                        }
                    });
        });
    }

    private void btcSubmitTransaction(Pointer context, BRWalletManager managerImpl, BRWallet walletImpl,
                                      BRTransaction transactionImpl, int rid) {
        systemExecutor.submit(() -> {
            Log.d(TAG, "BRSubmitTransactionCallback");

            CoreBRTransaction transaction = CoreBRTransaction.create(transactionImpl);
            query.putTransaction(getNetwork().getUids(), transaction.serialize(), new CompletionHandler<Transaction>() {
                @Override
                public void handleData(Transaction data) {
                    Log.d(TAG, "BRSubmitTransactionCallback: succeeded");
                    managerImpl.announceSubmit(rid, transaction, 0);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.d(TAG, "BRSubmitTransactionCallback: failed", error);
                    managerImpl.announceSubmit(rid, transaction, 1);
                }
            });
        });
    }

    private List<String> getAllAddrs(BRWalletManager manager) {
        final UnsignedInteger UNUSED_ADDR_LIMIT = UnsignedInteger.valueOf(25);

        manager.generateUnusedAddrs(UNUSED_ADDR_LIMIT);
        List<String> addrs = new ArrayList<>(manager.getAllAddrs());
        addrs.addAll(manager.getAllAddrsLegacy());
        return addrs;
    }

    // ETH client

    // TODO(fix): Implement me!

    private void ethGetBalance(Pointer context, Pointer ewm, Pointer wid, String address, int rid) {

    }

    private void ethGetGasPrice(Pointer context, Pointer ewm, Pointer wid, int rid) {

    }

    private void ethEstimateGas(Pointer context, Pointer ewm, Pointer wid, Pointer tid, String from, String to,
                                String amount, String data, int rid) {

    }

    private void ethSubmitTransaction(Pointer context, Pointer ewm, Pointer wid, Pointer tid, String transaction,
                                      int rid) {

    }

    private void ethGetTransactions(Pointer context, Pointer ewm, String address, long begBlockNumber,
                                    long endBlockNumber, int rid) {

    }

    private void ethGetLogs(Pointer context, Pointer ewm, String contract, String address, String event,
                            long begBlockNumber,
                            long endBlockNumber, int rid) {

    }

    private void ethGetBlocks(Pointer context, Pointer ewm, String address, int interests, long blockNumberStart,
                              long blockNumberStop, int rid) {

    }

    private void ethGetTokens(Pointer context, Pointer ewm, int rid) {

    }

    private void ethGetBlockNumber(Pointer context, Pointer ewm, int rid) {

    }

    private void ethGetNonce(Pointer context, Pointer ewm, String address, int rid) {

    }
}
