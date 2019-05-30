/*
 * WalletManager
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.bitcoin.BRWallet;
import com.breadwallet.crypto.jni.bitcoin.BRWalletManager;
import com.breadwallet.crypto.jni.bitcoin.CoreBRTransaction;
import com.google.common.base.Optional;

import java.util.List;

public abstract class WalletManager {

    /* package */
    abstract boolean matches(BRWalletManager walletManagerImpl);

    /* package */
    abstract Optional<Wallet> getOrCreateWalletByImpl(BRWallet walletImpl, boolean createAllowed);

    /* package */
    abstract WalletManagerState setState(WalletManagerState state);

    /* package */
    abstract List<String> getUnusedAddrsLegacy(int limit);

    /* package */
    abstract void announceBlockNumber(int rid, long blockNumber);

    /* package */
    abstract void announceSubmit(int rid, Transfer transfer, int errorCode);

    /* package */
    abstract void announceTransaction(int rid, CoreBRTransaction transaction);

    /* package */
    abstract void announceTransactionComplete(int rid, boolean success);

    public abstract void initialize();

    public abstract void connect();

    public abstract void disconnect();

    public abstract void sync();

    public abstract void submit(Transfer transfer, String paperKey);

    public abstract Account getAccount();

    public abstract Network getNetwork();

    public abstract Wallet getPrimaryWallet();

    public abstract List<Wallet> getWallets();

    public abstract WalletManagerMode getMode();

    public abstract String getPath();

    public abstract WalletManagerState getState();

    public Currency getCurrency() {
        return getNetwork().getCurrency();
    }

    public String getName() {
        return getCurrency().getCode();
    }

    public Unit getBaseUnit() {
        Network network = getNetwork();
        return network.baseUnitFor(network.getCurrency()).get();
    }

    public Unit getDefaultUnit() {
        Network network = getNetwork();
        return network.defaultUnitFor(network.getCurrency()).get();
    }

    public boolean isActive() {
        WalletManagerState state = getState();
        return state == WalletManagerState.CREATED || state == state.SYNCHING;
    }
}
