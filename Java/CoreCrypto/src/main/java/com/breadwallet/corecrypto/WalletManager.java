package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;
import android.util.Log;

import com.breadwallet.corenative.bitcoin.BRTransaction;
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
import com.breadwallet.corenative.crypto.CoreBRCryptoAmount;
import com.breadwallet.corenative.crypto.CoreBRCryptoFeeBasis;
import com.breadwallet.corenative.crypto.CoreBRCryptoTransfer;
import com.breadwallet.corenative.crypto.CoreBRCryptoWallet;
import com.breadwallet.corenative.crypto.CoreBRCryptoWalletManager;
import com.breadwallet.corenative.ethereum.BREthereumEwm;
import com.breadwallet.corenative.ethereum.BREthereumNetwork;
import com.breadwallet.corenative.ethereum.BREthereumToken;
import com.breadwallet.corenative.ethereum.BREthereumTransfer;
import com.breadwallet.corenative.ethereum.BREthereumWallet;
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

    private Optional<Wallet> getWallet(CoreBRCryptoWallet wallet) {
        return core.containsWallet(wallet) ?
                Optional.of(Wallet.create(wallet, this, networkBaseUnit, networkDefaultUnit)):
                Optional.absent();
    }

    private Optional<Wallet> getWalletOrCreate(CoreBRCryptoWallet wallet) {
        Optional<Wallet> optional = getWallet(wallet);
        if (optional.isPresent()) {
            return optional;

        } else {
            return Optional.of(Wallet.create(wallet, this, networkBaseUnit, networkDefaultUnit));
        }
    }

    private void walletManagerEventCallback(Pointer context, @Nullable BRCryptoWalletManager cryptoWalletManager,
                                            BRCryptoWalletManagerEvent.ByValue event) {
        Log.d(TAG, "WalletManagerEventCallback");

        core = CoreBRCryptoWalletManager.create(cryptoWalletManager);

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

        network.setHeight(blockHeight);
        announcer.announceWalletManagerEvent(this, new WalletManagerBlockUpdatedEvent(blockHeight));
    }

    private void walletEventCallback(Pointer context, @Nullable BRCryptoWalletManager cryptoWalletManager,
                                     @Nullable BRCryptoWallet cryptoWallet, BRCryptoWalletEvent.ByValue event) {
        Log.d(TAG, "WalletEventCallback");

        // take ownership of the wallet manager, even though it isn't used
        CoreBRCryptoWalletManager.create(cryptoWalletManager);
        CoreBRCryptoWallet wallet = CoreBRCryptoWallet.create(cryptoWallet);

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

    private void transferEventCallback(Pointer context, @Nullable BRCryptoWalletManager cryptoWalletManager,
                                       @Nullable BRCryptoWallet cryptoWallet, @Nullable BRCryptoTransfer cryptoTransfer,
                                       BRCryptoTransferEvent.ByValue event) {
        Log.d(TAG, "TransferEventCallback");

        // take ownership of the wallet manager, even though it isn't used
        CoreBRCryptoWalletManager.create(cryptoWalletManager);
        CoreBRCryptoWallet wallet = CoreBRCryptoWallet.create(cryptoWallet);
        CoreBRCryptoTransfer transfer = CoreBRCryptoTransfer.create(cryptoTransfer);

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

    private void btcGetBlockNumber(Pointer context, BRWalletManager managerImpl, int rid) {
        Log.d(TAG, "BRGetBlockNumberCallback");
        query.getBlockchain(getNetwork().getUids(), new CompletionHandler<Blockchain>() {
            @Override
            public void handleData(Blockchain blockchain) {
                UnsignedLong blockchainHeight = blockchain.getBlockHeight();
                Log.d(TAG, String.format("BRGetBlockNumberCallback: succeeded (%s)", blockchainHeight));
                managerImpl.announceBlockNumber(rid, blockchainHeight);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BRGetBlockNumberCallback: failed", error);
            }
        });
    }

    private void btcGetTransactions(Pointer context, BRWalletManager managerImpl, long begBlockNumber,
                                    long endBlockNumber, int rid) {
        systemExecutor.submit(() -> {
            UnsignedLong begBlockNumberUnsigned = UnsignedLong.fromLongBits(begBlockNumber);
            UnsignedLong endBlockNumberUnsigned = UnsignedLong.fromLongBits(endBlockNumber);

            Log.d(TAG, String.format("BRGetTransactionsCallback (%s -> %s)", begBlockNumberUnsigned, endBlockNumberUnsigned));

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
                            Log.e(TAG, "BRGetTransactionsCallback received an error, completing with failure", error);
                            managerImpl.announceTransactionComplete(rid, false);
                        }
                    });
        });
    }

    private void btcSubmitTransaction(Pointer context, BRWalletManager managerImpl, Pointer walletImpl,
                                      BRTransaction transactionImpl, int rid) {
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
                Log.e(TAG, "BRSubmitTransactionCallback: failed", error);
                managerImpl.announceSubmit(rid, transaction, 1);
            }
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

    private void ethGetBalance(Pointer context, BREthereumEwm ewm, BREthereumWallet wid, String address, int rid) {
        Log.d(TAG, "BREthereumClientHandlerGetBalance");

        BREthereumNetwork network = ewm.getNetwork();
        String networkName = network.getName().toLowerCase();

        BREthereumToken token = ewm.getToken(wid);
        if (null == token) {
            query.getBalanceAsEth(networkName, address, rid, new CompletionHandler<String>() {
                @Override
                public void handleData(String data) {
                    Log.d(TAG, "BREthereumClientHandlerGetBalance: asEth succeeded");
                    ewm.announceWalletBalance(wid, data, rid);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BREthereumClientHandlerGetBalance: asEth failed", error);
                }
            });
        } else {
            String tokenAddress = token.getAddress();
            query.getBalanceAsTok(networkName, address, tokenAddress, rid, new CompletionHandler<String>() {
                @Override
                public void handleData(String balance) {
                    Log.d(TAG, "BREthereumClientHandlerGetBalance: asTok succeeded");
                    ewm.announceWalletBalance(wid, balance, rid);
                }

                @Override
                public void handleError(QueryError error) {
                    Log.e(TAG, "BREthereumClientHandlerGetBalance: asTok failed", error);
                }
            });
        }
    }

    private void ethGetGasPrice(Pointer context, BREthereumEwm ewm, BREthereumWallet wid, int rid) {
        Log.d(TAG, "BREthereumClientHandlerGetGasPrice");

        BREthereumNetwork network = ewm.getNetwork();
        String networkName = network.getName().toLowerCase();

        query.getGasPriceAsEth(networkName, rid, new CompletionHandler<String>() {
            @Override
            public void handleData(String gasPrice) {
                Log.d(TAG, "BREthereumClientHandlerGetGasPrice: succeeded");
                ewm.announceGasPrice(wid, gasPrice, rid);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BREthereumClientHandlerGetGasPrice: failed", error);
            }
        });
    }

    private void ethEstimateGas(Pointer context, BREthereumEwm ewm, BREthereumWallet wid, BREthereumTransfer tid,
                                String from, String to,
                                String amount, String data, int rid) {
        Log.d(TAG, "BREthereumClientHandlerEstimateGas");

        BREthereumNetwork network = ewm.getNetwork();
        String networkName = network.getName().toLowerCase();

        query.getGasEstimateAsEth(networkName, from, to, amount, data, rid, new CompletionHandler<String>() {
            @Override
            public void handleData(String gasEstimate) {
                Log.d(TAG, "BREthereumClientHandlerEstimateGas: succeeded");
                ewm.announceGasEstimate(wid, tid, gasEstimate, rid);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BREthereumClientHandlerEstimateGas: failed", error);
            }
        });
    }

    private void ethSubmitTransaction(Pointer context, BREthereumEwm ewm, BREthereumWallet wid,
                                      BREthereumTransfer tid, String transaction,
                                      int rid) {
        Log.d(TAG, "BREthereumClientHandlerSubmitTransaction");

        BREthereumNetwork network = ewm.getNetwork();
        String networkName = network.getName().toLowerCase();

        query.submitTransactionAsEth(networkName, transaction, rid, new CompletionHandler<String>() {
            @Override
            public void handleData(String hash) {
                Log.d(TAG, "BREthereumClientHandlerSubmitTransaction: succeeded");
                // TODO(fix): The swift is populating default values; what is the right behaviour?
                ewm.announceSubmitTransfer(wid, tid, hash, -1, null, rid);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BREthereumClientHandlerSubmitTransaction: failed", error);
            }
        });
    }

    private void ethGetTransactions(Pointer context, BREthereumEwm ewm, String address, long begBlockNumber,
                                    long endBlockNumber, int rid) {
        Log.d(TAG, "BREthereumClientHandlerGetTransactions");

        BREthereumNetwork network = ewm.getNetwork();
        String networkName = network.getName().toLowerCase();

        query.getTransactionsAsEth(networkName, address, UnsignedLong.fromLongBits(begBlockNumber),
                UnsignedLong.fromLongBits(endBlockNumber), rid, new CompletionHandler<List<EthTransaction>>() {
                    @Override
                    public void handleData(List<EthTransaction> transactions) {
                        Log.d(TAG, "BREthereumClientHandlerGetTransactions: succeeded");
                        for (EthTransaction tx : transactions) {
                            ewm.announceTransaction(rid,
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
                        ewm.announceTransactionComplete(rid, true);
                    }

                    @Override
                    public void handleError(QueryError error) {
                        Log.e(TAG, "BREthereumClientHandlerGetTransactions: failed", error);
                        ewm.announceTransactionComplete(rid, false);
                    }
                });
    }

    private void ethGetLogs(Pointer context, BREthereumEwm ewm, String contract, String address, String event,
                            long begBlockNumber,
                            long endBlockNumber, int rid) {
        Log.d(TAG, "BREthereumClientHandlerGetLogs");

        BREthereumNetwork network = ewm.getNetwork();
        String networkName = network.getName().toLowerCase();

        query.getLogsAsEth(networkName, contract, address, event, UnsignedLong.fromLongBits(begBlockNumber),
                UnsignedLong.fromLongBits(endBlockNumber), rid, new CompletionHandler<List<EthLog>>() {
                    @Override
                    public void handleData(List<EthLog> logs) {
                        Log.d(TAG, "BREthereumClientHandlerGetLogs: succeeded");
                        for (EthLog log : logs) {
                            ewm.announceLog(rid,
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
                        ewm.announceLogComplete(rid, true);
                    }

                    @Override
                    public void handleError(QueryError error) {
                        Log.e(TAG, "BREthereumClientHandlerGetLogs: failed", error);
                        ewm.announceLogComplete(rid, false);
                    }
                });
    }

    private void ethGetBlocks(Pointer context, BREthereumEwm ewm, String address, int interests, long blockNumberStart,
                              long blockNumberStop, int rid) {
        Log.d(TAG, "BREthereumClientHandlerGetBlocks");

        BREthereumNetwork network = ewm.getNetwork();
        String networkName = network.getName().toLowerCase();

        query.getBlocksAsEth(networkName, address, UnsignedInteger.fromIntBits(interests),
                UnsignedLong.fromLongBits(blockNumberStart), UnsignedLong.fromLongBits(blockNumberStop), rid,
                new CompletionHandler<List<UnsignedLong>>() {
            @Override
            public void handleData(List<UnsignedLong> blocks) {
                Log.d(TAG, "BREthereumClientHandlerGetBlocks: succeeded");
                ewm.announceBlocks(rid, blocks);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BREthereumClientHandlerGetBlocks: failed", error);
            }
        });
    }

    private void ethGetTokens(Pointer context, BREthereumEwm ewm, int rid) {
        Log.d(TAG, "BREthereumClientHandlerGetTokens");

        query.getTokensAsEth(rid, new CompletionHandler<List<EthToken>>() {
            @Override
            public void handleData(List<EthToken> tokens) {
                Log.d(TAG, "BREthereumClientHandlerGetTokens: succeeded");
                for (EthToken token: tokens) {
                    ewm.announceToken(rid,
                            token.getAddress(),
                            token.getSymbol(),
                            token.getName(),
                            token.getDescription(),
                            token.getDecimals(),
                            token.getDefaultGasLimit().orNull(),
                            token.getDefaultGasPrice().orNull());
                }
                ewm.announceTokenComplete(rid, true);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BREthereumClientHandlerGetTokens: failed", error);
                ewm.announceTokenComplete(rid, false);
            }
        });
    }

    private void ethGetBlockNumber(Pointer context, BREthereumEwm ewm, int rid) {
        Log.d(TAG, "BREthereumClientHandlerGetBlockNumber");

        BREthereumNetwork network = ewm.getNetwork();
        String networkName = network.getName().toLowerCase();

        query.getBlockNumberAsEth(networkName, rid, new CompletionHandler<String>() {
            @Override
            public void handleData(String number) {
                Log.d(TAG, "BREthereumClientHandlerGetBlockNumber: succeeded");
                ewm.announceBlockNumber(number, rid);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BREthereumClientHandlerGetBlockNumber: failed", error);
            }
        });
    }

    private void ethGetNonce(Pointer context, BREthereumEwm ewm, String address, int rid) {
        Log.d(TAG, "BREthereumClientHandlerGetNonce");

        BREthereumNetwork network = ewm.getNetwork();
        String networkName = network.getName().toLowerCase();

        query.getNonceAsEth(networkName, address, rid, new CompletionHandler<String>() {
            @Override
            public void handleData(String nonce) {
                Log.d(TAG, "BREthereumClientHandlerGetNonce: succeeded");
                ewm.announceNonce(address, nonce, rid);
            }

            @Override
            public void handleError(QueryError error) {
                Log.e(TAG, "BREthereumClientHandlerGetNonce: failed", error);
            }
        });
    }
}
