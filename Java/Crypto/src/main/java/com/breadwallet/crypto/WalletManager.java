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

import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.util.List;

public abstract class WalletManager {

    /* package */
    abstract Wallet createWalletByPtr(Pointer walletPtr);

    /* package */
    abstract Optional<Wallet> getWalletByPtr(Pointer walletPtr);

    /* package */
    abstract Pointer getPointer();

    /* package */
    abstract WalletManagerState setState(WalletManagerState state);

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
