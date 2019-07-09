package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;
import android.util.Log;

import com.breadwallet.corenative.crypto.BRCryptoCWMClient;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientBtc;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientEth;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientCallbackState;
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
import com.breadwallet.crypto.TransferState;
import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletManagerState;
import com.breadwallet.crypto.WalletState;
import com.breadwallet.crypto.blockchaindb.BlockchainDb;
import com.breadwallet.crypto.blockchaindb.CompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.breadwallet.crypto.blockchaindb.models.brd.EthToken;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;
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
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Objects;
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

    private final BRCryptoCWMListener.ByValue listener;
    private final BRCryptoCWMClient.ByValue client;

    private CoreBRCryptoWalletManager core;

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

        // TODO(fix): Unchecked get here
        this.networkBaseUnit = network.baseUnitFor(networkCurrency).get();
        this.networkDefaultUnit = network.defaultUnitFor(networkCurrency).get();

        this.systemExecutor = Executors.newSingleThreadExecutor();
        this.announcer = announcer;
        this.query = query;

        this.listener = new BRCryptoCWMListener.ByValue(Pointer.NULL,
                this::walletManagerEventCallback,
                this::walletEventCallback,
                this::transferEventCallback);

        // TODO(fix): Add generic client, once defined
        this.client = new BRCryptoCWMClient.ByValue(Pointer.NULL,
                new BRCryptoCWMClientBtc(
                        this::btcGetBlockNumber,
                        this::btcGetTransactions,
                        this::btcSubmitTransaction),
                new BRCryptoCWMClientEth(
                        this::ethGetEtherBalance,
                        this::ethGetTokenBalance,
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
    public void submit(com.breadwallet.crypto.Transfer transfer, byte[] phraseUtf8) {
        Transfer cryptoTransfer = Transfer.from(transfer);
        Wallet cryptoWallet = cryptoTransfer.getWallet();
        core.submit(cryptoWallet.getCoreBRCryptoWallet(), cryptoTransfer.getCoreBRCryptoTransfer(), phraseUtf8);
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
        return Wallet.create(core.getWallet(), this);
    }

    @Override
    public List<Wallet> getWallets() {
        List<Wallet> wallets = new ArrayList<>();

        UnsignedLong count = core.getWalletsCount();
        for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
            wallets.add(Wallet.create(core.getWallet(i), this));
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

    private Optional<Wallet> getWallet(CoreBRCryptoWallet wallet) {
        return core.containsWallet(wallet) ?
                Optional.of(Wallet.create(wallet, this)):
                Optional.absent();
    }

    private Optional<Wallet> getWalletOrCreate(CoreBRCryptoWallet wallet) {
        Optional<Wallet> optional = getWallet(wallet);
        if (optional.isPresent()) {
            return optional;

        } else {
            return Optional.of(Wallet.create(wallet, this));
        }
    }

    private void walletManagerEventCallback(Pointer context, @Nullable BRCryptoWalletManager coreWalletManager,
                                            BRCryptoWalletManagerEvent.ByValue event) {
        Log.d(TAG, "WalletManagerEventCallback");

        core = CoreBRCryptoWalletManager.create(coreWalletManager);

        switch (event.type) {
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_CREATED: {
                handleWalletManagerCreated(event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_CHANGED: {
                handleWalletManagerChanged(event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_DELETED: {
                handleWalletManagerDeleted(event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED: {
                handleWalletManagerWalletAdded(event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED: {
                handleWalletManagerWalletChanged(event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED: {
                handleWalletManagerWalletDeleted(event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED: {
                handleWalletManagerSyncStarted(event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES: {
                handleWalletManagerSyncProgress(event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED: {
                handleWalletManagerSyncStopped(event);
                break;
            }
            case BRCryptoWalletManagerEventType.CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED: {
                handleWalletManagerBlockHeightUpdated(event);
                break;
            }
        }
    }

    private void handleWalletManagerCreated(BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerCreated");

        announcer.announceWalletManagerEvent(this, new WalletManagerCreatedEvent());
    }

    private void handleWalletManagerChanged(BRCryptoWalletManagerEvent event) {
        WalletManagerState oldState = Utilities.walletManagerStateFromCrypto(event.u.state.oldValue);
        WalletManagerState newState = Utilities.walletManagerStateFromCrypto(event.u.state.newValue);
        Log.d(TAG, String.format("WalletManagerChanged (%s -> %s)", oldState, newState));

        announcer.announceWalletManagerEvent(this, new WalletManagerChangedEvent(oldState, newState));
    }

    private void handleWalletManagerDeleted(BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerDeleted");

        announcer.announceWalletManagerEvent(this, new WalletManagerDeletedEvent());
    }

    private void handleWalletManagerWalletAdded(BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerWalletAdded");

        CoreBRCryptoWallet coreWallet = CoreBRCryptoWallet.create(event.u.wallet.value);

        Optional<Wallet> optional = getWallet(coreWallet);
        if (optional.isPresent()) {
            Wallet wallet = optional.get();
            announcer.announceWalletManagerEvent(this, new WalletManagerWalletAddedEvent(wallet));

        } else {
            Log.e(TAG, "WalletManagerWalletAdded: missed wallet");
        }
    }

    private void handleWalletManagerWalletChanged(BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerWalletChanged");

        CoreBRCryptoWallet coreWallet = CoreBRCryptoWallet.create(event.u.wallet.value);

        Optional<Wallet> optional = getWallet(coreWallet);
        if (optional.isPresent()) {
            Wallet wallet = optional.get();
            announcer.announceWalletManagerEvent(this, new WalletManagerWalletChangedEvent(wallet));

        } else {
            Log.e(TAG, "WalletManagerWalletChanged: missed wallet");
        }
    }

    private void handleWalletManagerWalletDeleted(BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerWalletDeleted");

        CoreBRCryptoWallet coreWallet = CoreBRCryptoWallet.create(event.u.wallet.value);

        Optional<Wallet> optional = getWallet(coreWallet);
        if (optional.isPresent()) {
            Wallet wallet = optional.get();
            announcer.announceWalletManagerEvent(this, new WalletManagerWalletDeletedEvent(wallet));

        } else {
            Log.e(TAG, "WalletManagerWalletDeleted: missed wallet");
        }
    }

    private void handleWalletManagerSyncStarted(BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerSyncStarted");

        announcer.announceWalletManagerEvent(this, new WalletManagerSyncStartedEvent());
    }

    private void handleWalletManagerSyncProgress(BRCryptoWalletManagerEvent event) {
        double percent = event.u.sync.percentComplete;
        Log.d(TAG, String.format("WalletManagerSyncProgress (%s)", percent));

        announcer.announceWalletManagerEvent(this, new WalletManagerSyncProgressEvent(percent));
    }

    private void handleWalletManagerSyncStopped(BRCryptoWalletManagerEvent event) {
        Log.d(TAG, "WalletManagerSyncStopped");
        // TODO(fix): fill in message
        announcer.announceWalletManagerEvent(this, new WalletManagerSyncStoppedEvent(""));
    }

    private void handleWalletManagerBlockHeightUpdated(BRCryptoWalletManagerEvent event) {
        UnsignedLong blockHeight = UnsignedLong.fromLongBits(event.u.blockHeight.value);
        Log.d(TAG, String.format("WalletManagerBlockHeightUpdated (%s)", blockHeight));

        announcer.announceWalletManagerEvent(this, new WalletManagerBlockUpdatedEvent(blockHeight));
    }

    private void walletEventCallback(Pointer context, @Nullable BRCryptoWalletManager coreWalletManager,
                                     @Nullable BRCryptoWallet coreWallet, BRCryptoWalletEvent.ByValue event) {
        Log.d(TAG, "WalletEventCallback");

        // take ownership of the wallet manager, even though it isn't used
        CoreBRCryptoWalletManager.create(coreWalletManager);
        CoreBRCryptoWallet wallet = CoreBRCryptoWallet.create(coreWallet);

        switch (event.type) {
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_CREATED: {
                handleWalletCreated(wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_CHANGED: {
                handleWalletChanged(wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_DELETED: {
                handleWalletDeleted(wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_ADDED: {
                handleWalletTransferAdded(wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_CHANGED: {
                handleWalletTransferChanged(wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED: {
                handleWalletTransferSubmitted(wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_TRANSFER_DELETED: {
                handleWalletTransferDeleted(wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_BALANCE_UPDATED: {
                handleWalletBalanceUpdated(wallet, event);
                break;
            }
            case BRCryptoWalletEventType.CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED: {
                handleWalletFeeBasisUpdate(wallet, event);
                break;
            }
        }
    }

    private void handleWalletCreated(CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletCreated");

        Optional<Wallet> optWallet = getWalletOrCreate(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();
            announcer.announceWalletEvent(this, wallet, new WalletCreatedEvent());

        } else {
            Log.e(TAG, "WalletCreated: missed wallet");
        }

    }

    private void handleWalletChanged(CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        WalletState oldState = Utilities.walletStateFromCrypto(event.u.state.oldState);
        WalletState newState = Utilities.walletStateFromCrypto(event.u.state.newState);
        Log.d(TAG, String.format("WalletChanged (%s -> %s)", oldState, newState));

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();
            announcer.announceWalletEvent(this, wallet, new WalletChangedEvent(oldState, newState));

        } else {
            Log.e(TAG, "WalletChanged: missed wallet");
        }
    }

    private void handleWalletDeleted(CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletDeleted");

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();
            announcer.announceWalletEvent(this, wallet, new WalletDeletedEvent());

        } else {
            Log.e(TAG, "WalletDeleted: missed wallet");
        }
    }

    private void handleWalletTransferAdded(CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletTransferAdded");

        CoreBRCryptoTransfer coreTransfer = CoreBRCryptoTransfer.create(event.u.transfer.value);

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();
            Optional<Transfer> optional = wallet.getTransfer(coreTransfer);

            if (optional.isPresent()) {
                Transfer transfer = optional.get();
                announcer.announceWalletEvent(this, wallet, new WalletTransferAddedEvent(transfer));

            } else {
                Log.e(TAG, "WalletTransferAdded: missed transfer");
            }

        } else {
            Log.e(TAG, "WalletTransferAdded: missed wallet");
        }
    }

    private void handleWalletTransferChanged(CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletTransferChanged");

        CoreBRCryptoTransfer coreTransfer = CoreBRCryptoTransfer.create(event.u.transfer.value);

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();
            Optional<Transfer> optional = wallet.getTransfer(coreTransfer);

            if (optional.isPresent()) {
                Transfer transfer = optional.get();
                announcer.announceWalletEvent(this, wallet, new WalletTransferChangedEvent(transfer));

            } else {
                Log.e(TAG, "WalletTransferChanged: missed transfer");
            }

        } else {
            Log.e(TAG, "WalletTransferChanged: missed wallet");
        }
    }

    private void handleWalletTransferSubmitted(CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletTransferSubmitted");

        CoreBRCryptoTransfer coreTransfer = CoreBRCryptoTransfer.create(event.u.transfer.value);

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();
            Optional<Transfer> optional = wallet.getTransfer(coreTransfer);

            if (optional.isPresent()) {
                Transfer transfer = optional.get();
                announcer.announceWalletEvent(this, wallet, new WalletTransferSubmittedEvent(transfer));

            } else {
                Log.e(TAG, "WalletTransferSubmitted: missed transfer");
            }

        } else {
            Log.e(TAG, "WalletTransferSubmitted: missed wallet");
        }
    }

    private void handleWalletTransferDeleted(CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletTransferDeleted");

        CoreBRCryptoTransfer coreTransfer = CoreBRCryptoTransfer.create(event.u.transfer.value);

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();
            Optional<Transfer> optional = wallet.getTransfer(coreTransfer);

            if (optional.isPresent()) {
                Transfer transfer = optional.get();
                announcer.announceWalletEvent(this, wallet, new WalletTransferDeletedEvent(transfer));

            } else {
                Log.e(TAG, "WalletTransferDeleted: missed transfer");
            }

        } else {
            Log.e(TAG, "WalletTransferDeleted: missed wallet");
        }
    }

    private void handleWalletBalanceUpdated(CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletBalanceUpdated");

        CoreBRCryptoAmount coreAmount = CoreBRCryptoAmount.createOwned(event.u.balanceUpdated.amount);

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();
            Amount amount = Amount.create(coreAmount, wallet.getBaseUnit());
            Log.d(TAG, String.format("WalletBalanceUpdated: %s", amount));
            announcer.announceWalletEvent(this, wallet, new WalletBalanceUpdatedEvent(amount));

        } else {
            Log.e(TAG, "WalletBalanceUpdated: missed wallet");
        }
    }

    private void handleWalletFeeBasisUpdate(CoreBRCryptoWallet coreWallet, BRCryptoWalletEvent event) {
        Log.d(TAG, "WalletFeeBasisUpdate");

        CoreBRCryptoFeeBasis coreFeeBasis = CoreBRCryptoFeeBasis.createOwned(event.u.feeBasisUpdated.basis);

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();
            TransferFeeBasis feeBasis = TransferFeeBasis.create(coreFeeBasis);
            Log.d(TAG, String.format("WalletFeeBasisUpdate: %s", feeBasis));
            announcer.announceWalletEvent(this, wallet, new WalletFeeBasisUpdatedEvent(feeBasis));

        } else {
            Log.e(TAG, "WalletFeeBasisUpdate: missed wallet");
        }
    }

    private void transferEventCallback(Pointer context, @Nullable BRCryptoWalletManager coreWalletManager,
                                       @Nullable BRCryptoWallet coreWallet, @Nullable BRCryptoTransfer coreTransfer,
                                       BRCryptoTransferEvent.ByValue event) {
        Log.d(TAG, "TransferEventCallback");

        // take ownership of the wallet manager, even though it isn't used
        CoreBRCryptoWalletManager.create(coreWalletManager);
        CoreBRCryptoWallet wallet = CoreBRCryptoWallet.create(coreWallet);
        CoreBRCryptoTransfer transfer = CoreBRCryptoTransfer.create(coreTransfer);

        switch (event.type) {
            case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_CREATED: {
                handleTransferCreated(wallet, transfer, event);
                break;
            }
            case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_CHANGED: {
                handleTransferChanged(wallet, transfer, event);
                break;
            }
            case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_CONFIRMED: {
                handleTransferConfirmed(wallet, transfer, event);
                break;
            }
            case BRCryptoTransferEventType.CRYPTO_TRANSFER_EVENT_DELETED: {
                handleTransferDeleted(wallet, transfer, event);
                break;
            }
        }
    }

    private void handleTransferCreated(CoreBRCryptoWallet coreWallet, CoreBRCryptoTransfer coreTransfer,
                                       BRCryptoTransferEvent event) {
        Log.d(TAG, "TransferCreated");

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();

            Optional<Transfer> optTransfer = wallet.getTransferOrCreate(coreTransfer);
            if (optTransfer.isPresent()) {
                Transfer transfer = optTransfer.get();
                announcer.announceTransferEvent(this, wallet, transfer, new TransferCreatedEvent());

            } else {
                Log.e(TAG, "TransferCreated: missed transfer");
            }

        } else {
            Log.e(TAG, "TransferCreated: missed wallet");
        }
    }

    private void handleTransferConfirmed(CoreBRCryptoWallet coreWallet, CoreBRCryptoTransfer coreTransfer,
                                         BRCryptoTransferEvent event) {
        UnsignedLong confirmations = UnsignedLong.fromLongBits(event.u.confirmation.count);
        Log.d(TAG, String.format("TransferConfirmed (%s)", confirmations));

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();

            Optional<Transfer> optTransfer = wallet.getTransfer(coreTransfer);
            if (optTransfer.isPresent()) {
                Transfer transfer = optTransfer.get();
                announcer.announceTransferEvent(this, wallet, transfer, new TransferConfirmationEvent(confirmations));

            } else {
                Log.e(TAG, "TransferConfirmed: missed transfer");
            }

        } else {
            Log.e(TAG, "TransferConfirmed: missed wallet");
        }
    }

    private void handleTransferChanged(CoreBRCryptoWallet coreWallet, CoreBRCryptoTransfer coreTransfer,
                                       BRCryptoTransferEvent event) {
        // TODO(fix): Deal with memory management for the fee
        TransferState oldState = Utilities.transferStateFromCrypto(event.u.state.oldState);
        TransferState newState = Utilities.transferStateFromCrypto(event.u.state.newState);
        Log.d(TAG, String.format("TransferChanged (%s -> %s)", oldState, newState));

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();

            Optional<Transfer> optTransfer = wallet.getTransfer(coreTransfer);
            if (optTransfer.isPresent()) {
                Transfer transfer = optTransfer.get();

                announcer.announceTransferEvent(this, wallet, transfer, new TransferChangedEvent(oldState, newState));

            } else {
                Log.e(TAG, "TransferChanged: missed transfer");
            }

        } else {
            Log.e(TAG, "TransferChanged: missed wallet");
        }
    }

    private void handleTransferDeleted(CoreBRCryptoWallet coreWallet, CoreBRCryptoTransfer coreTransfer,
                                       BRCryptoTransferEvent event) {
        Log.d(TAG, "TransferDeleted");

        Optional<Wallet> optWallet = getWallet(coreWallet);
        if (optWallet.isPresent()) {
            Wallet wallet = optWallet.get();

            Optional<Transfer> optTransfer = wallet.getTransfer(coreTransfer);
            if (optTransfer.isPresent()) {
                Transfer transfer = optTransfer.get();
                announcer.announceTransferEvent(this, wallet, transfer, new TransferDeletedEvent());

            } else {
                Log.e(TAG, "TransferDeleted: missed transfer");
            }

        } else {
            Log.e(TAG, "TransferDeleted: missed wallet");
        }
    }

    // BTC client

    private void btcGetBlockNumber(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState) {
        Log.d(TAG, "BRCryptoCWMBtcGetBlockNumberCallback");

        query.getBlockchain(getNetwork().getUids(), new CompletionHandler<Blockchain>() {
            @Override
            public void handleData(Blockchain blockchain) {
                UnsignedLong blockchainHeight = blockchain.getBlockHeight();
                Log.d(TAG, String.format("BRCryptoCWMBtcGetBlockNumberCallback: succeeded (%s)", blockchainHeight));
                manager.announceGetBlockNumberSuccess(callbackState, blockchainHeight);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRCryptoCWMBtcGetBlockNumberCallback: failed", error);
                manager.announceGetBlockNumberFailure(callbackState);
            }
        });
    }

    private void btcGetTransactions(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                    Pointer addrs, SizeT addrCount, long begBlockNumber, long endBlockNumber) {
        UnsignedLong begBlockNumberUnsigned = UnsignedLong.fromLongBits(begBlockNumber);
        UnsignedLong endBlockNumberUnsigned = UnsignedLong.fromLongBits(endBlockNumber);

        Log.d(TAG, String.format("BRCryptoCWMBtcGetTransactionsCallback (%s -> %s)", begBlockNumberUnsigned,
                endBlockNumberUnsigned));

        int addressesCount = UnsignedInts.checkedCast(addrCount.longValue());
        String[] addresses = addrs.getStringArray(0, addressesCount, "UTF-8");

        query.getTransactions(getNetwork().getUids(), Arrays.asList(addresses), begBlockNumberUnsigned,
                endBlockNumberUnsigned, true,
                false, new CompletionHandler<List<Transaction>>() {
                    @Override
                    public void handleData(List<Transaction> transactions) {
                        Log.d(TAG, "BRCryptoCWMBtcGetTransactionsCallback received transactions");

                        for (Transaction transaction : transactions) {
                            Optional<byte[]> optRaw = transaction.getRaw();
                            if (!optRaw.isPresent()) {
                                Log.e(TAG, "BRCryptoCWMBtcGetTransactionsCallback completing with missing raw bytes");
                                manager.announceGetTransactionsComplete(callbackState, false);
                                return;
                            }

                            UnsignedLong blockHeight = transaction.getBlockHeight().or(UnsignedLong.ZERO);
                            UnsignedLong timestamp =
                                    transaction.getTimestamp().transform(Date::getTime).transform(TimeUnit.MILLISECONDS::toSeconds).transform(UnsignedLong::valueOf).or(UnsignedLong.ZERO);
                            Log.d(TAG,
                                    "BRCryptoCWMBtcGetTransactionsCallback announcing " + transaction.getId());
                            manager.announceGetTransactionsItemBtc(callbackState, optRaw.get(), timestamp, blockHeight);
                        }

                        Log.d(TAG, "BRCryptoCWMBtcGetTransactionsCallback: complete");
                        manager.announceGetTransactionsComplete(callbackState, true);
                    }

                    @Override
                    public void handleError(QueryError error) {
                        Log.e(TAG, "BRCryptoCWMBtcGetTransactionsCallback received an error, completing with failure", error);
                        manager.announceGetTransactionsComplete(callbackState, false);
                    }
                });
    }

    private void btcSubmitTransaction(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                      Pointer tx, SizeT txLength) {
        Log.d(TAG, "BRCryptoCWMBtcSubmitTransactionCallback");

        int transactionLength = UnsignedInts.checkedCast(txLength.longValue());
        byte[] transaction = tx.getByteArray(0, transactionLength);

        query.putTransaction(getNetwork().getUids(), transaction, new CompletionHandler<Transaction>() {
            @Override
            public void handleData(Transaction data) {
                Log.d(TAG, "BRCryptoCWMBtcSubmitTransactionCallback: succeeded");
                manager.announceSubmitTransferSuccess(callbackState);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRCryptoCWMBtcSubmitTransactionCallback: failed", error);
                manager.announceSubmitTransferFailure(callbackState);
            }
        });
    }

    // ETH client

    private void ethGetEtherBalance(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                    String networkName, String address) {
        Log.d(TAG, "BRCryptoCWMEthGetEtherBalanceCallback");

        query.getBalanceAsEth(networkName, address, new CompletionHandler<String>() {
            @Override
            public void handleData(String balance) {
                Log.d(TAG, "BRCryptoCWMEthGetEtherBalanceCallback: succeeded");
                manager.announceGetBalanceSuccess(callbackState, balance);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRCryptoCWMEthGetEtherBalanceCallback: failed", error);
                manager.announceGetBalanceFailure(callbackState);
            }
        });
    }

    private void ethGetTokenBalance(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                    String networkName, String address, String tokenAddress) {
        Log.d(TAG, "BRCryptoCWMEthGetTokenBalanceCallback");

        query.getBalanceAsTok(networkName, address, tokenAddress, new CompletionHandler<String>() {
            @Override
            public void handleData(String balance) {
                Log.d(TAG, "BRCryptoCWMEthGetTokenBalanceCallback: succeeded");
                manager.announceGetBalanceSuccess(callbackState, balance);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRCryptoCWMEthGetTokenBalanceCallback: failed", error);
                manager.announceGetBalanceFailure(callbackState);
            }
        });
    }

    private void ethGetGasPrice(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                String networkName) {
        Log.d(TAG, "BRCryptoCWMEthGetGasPriceCallback");

        query.getGasPriceAsEth(networkName, new CompletionHandler<String>() {
            @Override
            public void handleData(String gasPrice) {
                Log.d(TAG, "BRCryptoCWMEthGetGasPriceCallback: succeeded");
                manager.announceGetGasPriceSuccess(callbackState, gasPrice);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRCryptoCWMEthGetGasPriceCallback: failed", error);
                manager.announceGetGasPriceFailure(callbackState);
            }
        });
    }

    private void ethEstimateGas(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                String networkName, String from, String to, String amount, String data) {
        Log.d(TAG, "BRCryptoCWMEthEstimateGasCallback");

        query.getGasEstimateAsEth(networkName, from, to, amount, data, new CompletionHandler<String>() {
            @Override
            public void handleData(String gasEstimate) {
                Log.d(TAG, "BRCryptoCWMEthEstimateGasCallback: succeeded");
                manager.announceGetGasEstimateSuccess(callbackState, gasEstimate);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRCryptoCWMEthEstimateGasCallback: failed", error);
                manager.announceGetGasEstimateFailure(callbackState);
            }
        });
    }

    private void ethSubmitTransaction(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                      String networkName, String transaction) {
        Log.d(TAG, "BRCryptoCWMEthSubmitTransactionCallback");

        query.submitTransactionAsEth(networkName, transaction, new CompletionHandler<String>() {
            @Override
            public void handleData(String hash) {
                Log.d(TAG, "BRCryptoCWMEthSubmitTransactionCallback: succeeded");
                manager.announceSubmitTransferSuccess(callbackState, hash);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRCryptoCWMEthSubmitTransactionCallback: failed", error);
                manager.announceSubmitTransferFailure(callbackState);
            }
        });
    }

    private void ethGetTransactions(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                    String networkName, String address, long begBlockNumber, long endBlockNumber) {
        Log.d(TAG, "BRCryptoCWMEthGetTransactionsCallback");

        query.getTransactionsAsEth(networkName, address, UnsignedLong.fromLongBits(begBlockNumber),
                UnsignedLong.fromLongBits(endBlockNumber), new CompletionHandler<List<EthTransaction>>() {
                    @Override
                    public void handleData(List<EthTransaction> transactions) {
                        Log.d(TAG, "BRCryptoCWMEthGetTransactionsCallback: succeeded");
                        for (EthTransaction tx : transactions) {
                            manager.announceGetTransactionsItemEth(callbackState,
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
                        manager.announceGetTransactionsComplete(callbackState, true);
                    }

                    @Override
                    public void handleError(QueryError error) {
                        Log.e(TAG, "BRCryptoCWMEthGetTransactionsCallback: failed", error);
                        manager.announceGetTransactionsComplete(callbackState, false);
                    }
                });
    }

    private void ethGetLogs(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                            String networkName, String contract, String address, String event, long begBlockNumber,
                            long endBlockNumber) {
        Log.d(TAG, "BRCryptoCWMEthGetLogsCallback");

        query.getLogsAsEth(networkName, contract, address, event, UnsignedLong.fromLongBits(begBlockNumber),
                UnsignedLong.fromLongBits(endBlockNumber), new CompletionHandler<List<EthLog>>() {
                    @Override
                    public void handleData(List<EthLog> logs) {
                        Log.d(TAG, "BRCryptoCWMEthGetLogsCallback: succeeded");
                        for (EthLog log : logs) {
                            manager.announceGetLogsItem(callbackState,
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
                        manager.announceGetLogsComplete(callbackState, true);
                    }

                    @Override
                    public void handleError(QueryError error) {
                        Log.e(TAG, "BRCryptoCWMEthGetLogsCallback: failed", error);
                        manager.announceGetLogsComplete(callbackState, false);
                    }
                });
    }

    private void ethGetBlocks(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                              String networkName, String address, int interests, long blockNumberStart,
                              long blockNumberStop) {
        Log.d(TAG, "BRCryptoCWMEthGetBlocksCallback");

        query.getBlocksAsEth(networkName, address, UnsignedInteger.fromIntBits(interests),
                UnsignedLong.fromLongBits(blockNumberStart), UnsignedLong.fromLongBits(blockNumberStop),
                new CompletionHandler<List<UnsignedLong>>() {
            @Override
            public void handleData(List<UnsignedLong> blocks) {
                Log.d(TAG, "BRCryptoCWMEthGetBlocksCallback: succeeded");
                manager.announceGetBlocksSuccess(callbackState, blocks);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRCryptoCWMEthGetBlocksCallback: failed", error);
                manager.announceGetBlocksFailure(callbackState);
            }
        });
    }

    private void ethGetTokens(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState) {
        Log.d(TAG, "BREthereumClientHandlerGetTokens");

        query.getTokensAsEth(new CompletionHandler<List<EthToken>>() {
            @Override
            public void handleData(List<EthToken> tokens) {
                Log.d(TAG, "BREthereumClientHandlerGetTokens: succeeded");
                for (EthToken token: tokens) {
                    manager.announceGetTokensItem(callbackState,
                            token.getAddress(),
                            token.getSymbol(),
                            token.getName(),
                            token.getDescription(),
                            token.getDecimals(),
                            token.getDefaultGasLimit().orNull(),
                            token.getDefaultGasPrice().orNull());
                }
                manager.announceGetTokensComplete(callbackState, true);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BREthereumClientHandlerGetTokens: failed", error);
                manager.announceGetTokensComplete(callbackState, false);
            }
        });
    }

    private void ethGetBlockNumber(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                                   String networkName) {
        Log.d(TAG, "BRCryptoCWMEthGetBlockNumberCallback");

        query.getBlockNumberAsEth(networkName, new CompletionHandler<String>() {
            @Override
            public void handleData(String number) {
                Log.d(TAG, "BRCryptoCWMEthGetBlockNumberCallback: succeeded");
                manager.announceGetBlockNumberSuccess(callbackState, number);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRCryptoCWMEthGetBlockNumberCallback: failed", error);
                manager.announceGetBlockNumberFailure(callbackState);
            }
        });
    }

    private void ethGetNonce(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                             String networkName, String address) {
        Log.d(TAG, "BRCryptoCWMEthGetNonceCallback");

        query.getNonceAsEth(networkName, address, new CompletionHandler<String>() {
            @Override
            public void handleData(String nonce) {
                Log.d(TAG, "BRCryptoCWMEthGetNonceCallback: succeeded");
                manager.announceGetNonceSuccess(callbackState, address, nonce);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRCryptoCWMEthGetNonceCallback: failed", error);
                manager.announceGetNonceFailure(callbackState);
            }
        });
    }
}
