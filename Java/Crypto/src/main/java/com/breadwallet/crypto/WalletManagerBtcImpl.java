package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.BRMasterPubKey;
import com.breadwallet.crypto.jni.BRWalletManagerClient;
import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRSyncMode;
import com.breadwallet.crypto.jni.CryptoLibrary.BRWallet;
import com.breadwallet.crypto.jni.CryptoLibrary.BRWalletManager;
import com.breadwallet.crypto.jni.CryptoLibrary.BRChainParams;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.sun.jna.Pointer;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

// TODO: Where do we free 'core'?

// WARNING: This class is NOT threadsafe!
/* package */
class WalletManagerBtcImpl extends WalletManager {

    private static int modeAsBtc(WalletManagerMode  mode) {
        switch (mode) {
            case API_ONLY: return BRSyncMode.SYNC_MODE_BRD_ONLY;
            case P2P_ONLY: return BRSyncMode.SYNC_MODE_P2P_ONLY;
            case P2P_WITH_API_SYNC: return BRSyncMode.SYNC_MODE_P2P_WITH_BRD_SYNC;
            case API_WITH_P2P_SUBMIT: return BRSyncMode.SYNC_MODE_BRD_WITH_P2P_SEND;
            default: throw new IllegalStateException("Invalid mode");
        }
    }

    private final List<Wallet> wallets;

    private final BRWalletManager core;

    private final Account account;
    private final Network network;
    private final String path;
    private final WalletManagerMode mode;

    private WalletManagerState state;

    /* package */
    WalletManagerBtcImpl(BRWalletManagerClient.ByValue client, Account account, Network network,
                         WalletManagerMode managerMode, String path) {
        this.account = account;
        this.network = network;
        this.mode = managerMode;
        this.path = path;

        // TODO: This (assuming network.asBtc() not null, truncating timestamp) is how swift behaves, is this what we want?
        BRMasterPubKey.ByValue pubKey = account.asBtc();
        BRChainParams chainParams = network.asBtc().orNull();
        int timestamp = (int) account.getTimestamp();
        int mode = modeAsBtc(managerMode);

        this.core = CryptoLibrary.INSTANCE.BRWalletManagerNew(client, pubKey, chainParams, timestamp, mode, path);

        BRWallet wallet = CryptoLibrary.INSTANCE.BRWalletManagerGetWallet(core);
        checkNotNull(wallet);
        Pointer walletPtr = wallet.getPointer();
        checkNotNull(walletPtr);
        Wallet primaryWallet = new WalletBtcImpl(this, walletPtr, getBaseUnit(), getDefaultUnit());

        this.wallets = Collections.synchronizedList(new ArrayList<>(Collections.singleton(primaryWallet)));
    }

    @Override
    public void initialize() {
        // TODO: Add call to CryptoLibrary.INSTANCE.BRWalletManagerInit(core);
    }

    @Override
    public void connect() {
        CryptoLibrary.INSTANCE.BRWalletManagerConnect(core);
    }

    @Override
    public void disconnect() {
        CryptoLibrary.INSTANCE.BRWalletManagerDisconnect(core);
    }

    @Override
    public void sync() {
        CryptoLibrary.INSTANCE.BRWalletManagerScan(core);
    }

    @Override
    public void submit(Transfer transfer, String paperKey) {
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
        return wallets.get(0);
    }

    @Override
    public List<Wallet> getWallets() {
        return new ArrayList<>(wallets);
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
    public WalletManagerState getState() {
        return state;
    }

    @Override
    /* package */
    WalletManagerState setState(WalletManagerState newState) {
        // TODO: Do we want to synchronize here?
        WalletManagerState oldState = state;
        state = newState;
        return oldState;
    }

    @Override
    /* package */
    Optional<Wallet> getOrCreateWalletByPtr(Pointer walletPtr, boolean createAllowed) {
        Optional<Wallet> optWallet = getWalletByPtr(walletPtr);
        if (optWallet.isPresent()) {
            return optWallet;
        } else if (createAllowed) {
            return Optional.of(createWalletByPtr(walletPtr));
        } else {
            return Optional.absent();
        }
    }

    private
    Wallet createWalletByPtr(Pointer walletPtr) {
        Wallet wallet = new WalletBtcImpl(this, walletPtr, getBaseUnit(), getDefaultUnit());
        wallets.add(wallet);
        return wallet;
    }

    private
    Optional<Wallet> getWalletByPtr(Pointer walletPtr) {
        for (Wallet wallet: wallets) {
            if (wallet.getPointer().equals(walletPtr)) {
                return Optional.of(wallet);
            }
        }
        return Optional.absent();
    }

    @Override
    /* package */
    public Pointer getPointer() {
        return core.getPointer();
    }
}
