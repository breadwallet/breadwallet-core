package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.bitcoin.BRWalletManager;
import com.breadwallet.crypto.jni.bitcoin.CoreBRWalletManager;
import com.breadwallet.crypto.jni.support.BRMasterPubKey;
import com.breadwallet.crypto.jni.bitcoin.BRWalletManagerClient;
import com.breadwallet.crypto.jni.CryptoLibrary.BRSyncMode;
import com.breadwallet.crypto.jni.bitcoin.BRWallet;
import com.breadwallet.crypto.jni.bitcoin.BRChainParams;
import com.google.common.base.Optional;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

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

    private final CoreBRWalletManager core;

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
        BRChainParams chainParams = network.asBtc();
        int timestamp = (int) account.getTimestamp();
        int mode = modeAsBtc(managerMode);

        this.core = CoreBRWalletManager.create(client, pubKey, chainParams, timestamp, mode, path);

        BRWallet wallet = core.getWallet();
        checkNotNull(wallet);
        Wallet primaryWallet = new WalletBtcImpl(this, wallet, getBaseUnit(), getDefaultUnit());

        this.wallets = Collections.synchronizedList(new ArrayList<>(Collections.singleton(primaryWallet)));
    }

    @Override
    public void initialize() {
        // TODO: Add call to CryptoLibrary.INSTANCE.BRWalletManagerInit(core);
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
        core.scan();
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
    boolean matches(BRWalletManager walletManagerImpl) {
        return core.matches(walletManagerImpl);
    }

    @Override
    /* package */
    Optional<Wallet> getOrCreateWalletByImpl(BRWallet walletImpl, boolean createAllowed) {
        Optional<Wallet> optWallet = getWalletByImpl(walletImpl);
        if (optWallet.isPresent()) {
            return optWallet;
        } else if (createAllowed) {
            return Optional.of(createWalletByPtr(walletImpl));
        } else {
            return Optional.absent();
        }
    }

    private
    Wallet createWalletByPtr(BRWallet walletImpl) {
        Wallet wallet = new WalletBtcImpl(this, walletImpl, getBaseUnit(), getDefaultUnit());
        wallets.add(wallet);
        return wallet;
    }

    private
    Optional<Wallet> getWalletByImpl(BRWallet walletImpl) {
        for (Wallet wallet: wallets) {
            if (wallet.matches(walletImpl)) {
                return Optional.of(wallet);
            }
        }
        return Optional.absent();
    }
}
