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

import java.util.List;

public interface WalletManager {

    void initialize();

    void connect();

    void disconnect();

    void sync();

    void submit(Transfer transfer, String paperKey);

    default boolean isActive() {
        WalletManagerState state = getState();
        return state == WalletManagerState.CREATED || state == WalletManagerState.SYNCING;
    }

    Account getAccount();

    Network getNetwork();

    Wallet getPrimaryWallet();

    List<Wallet> getWallets();

    WalletManagerMode getMode();

    String getPath();

    default Currency getCurrency() {
        return getNetwork().getCurrency();
    }

    default String getName() {
        return getCurrency().getCode();
    }

    default Unit getBaseUnit() {
        Network network = getNetwork();
        return network.baseUnitFor(network.getCurrency()).get();
    }

    default Unit getDefaultUnit() {
        Network network = getNetwork();
        return network.defaultUnitFor(network.getCurrency()).get();
    }

    WalletManagerState getState();
}
