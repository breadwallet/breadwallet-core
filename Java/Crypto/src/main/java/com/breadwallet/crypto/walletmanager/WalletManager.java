/*
 * WalletManager
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.walletmanager;

import com.breadwallet.crypto.Account;
import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.network.Network;
import com.breadwallet.crypto.system.System;
import com.breadwallet.crypto.transfer.Transfer;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.wallet.Wallet;

import java.util.List;

public interface WalletManager {

    void connect();

    void disconnect();

    void submit(Transfer transfer, String paperKey);

    void sync();

    System getSystem();

    Account getAccount();

    Network getNetwork();

    Wallet getPrimaryWallet();

    List<Wallet> getWallets();

    WalletManagerMode getMode();

    String getPath();

    WalletManagerState getState();

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

    default boolean isActive() {
        WalletManagerState state = getState();
        return state == WalletManagerState.CREATED || state == state.SYNCHING;
    }
}
