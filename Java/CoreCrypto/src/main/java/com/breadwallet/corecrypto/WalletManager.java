/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.WalletManagerMode;
import com.breadwallet.crypto.WalletManagerState;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/* package */
abstract class WalletManager<T extends Wallet> implements com.breadwallet.crypto.WalletManager {

    protected final Account account;
    protected final Network network;
    protected final String path;
    protected final WalletManagerMode mode;
    protected final AtomicReference<WalletManagerState> state;
    protected final Lock walletsReadLock;
    protected final Lock walletsWriteLock;
    protected final List<T> wallets;

    protected WalletManager(Account account, Network network, WalletManagerMode managerMode, String path) {
        this.account = account;
        this.network = network;
        this.mode = managerMode;
        this.path = path;
        this.state = new AtomicReference<>(WalletManagerState.CREATED);

        ReadWriteLock walletsRwLock = new ReentrantReadWriteLock();
        this.walletsReadLock = walletsRwLock.readLock();
        this.walletsWriteLock = walletsRwLock.writeLock();
        this.wallets = new ArrayList<>();
    }

    @Override
    public com.breadwallet.crypto.Account getAccount() {
        return account;
    }

    @Override
    public com.breadwallet.crypto.Network getNetwork() {
        return network;
    }

    @Override
    public com.breadwallet.crypto.Wallet getPrimaryWallet() {
        walletsReadLock.lock();
        try {
            return wallets.get(0);
        } finally {
            walletsReadLock.unlock();
        }
    }

    @Override
    public List<com.breadwallet.crypto.Wallet> getWallets() {
        walletsReadLock.lock();
        try {
            return new ArrayList<>(wallets);
        } finally {
            walletsReadLock.unlock();
        }
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
        return state.get();
    }

    /* package */
    WalletManagerState setState(WalletManagerState newState) {
        return state.getAndSet(newState);
    }
}
