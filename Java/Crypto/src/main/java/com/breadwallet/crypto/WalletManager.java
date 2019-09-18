/*
 * WalletManager
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.errors.WalletSweeperError;
import com.breadwallet.crypto.utility.CompletionHandler;

import java.util.List;

public interface WalletManager {

    void createSweeper(Wallet wallet, Key key, CompletionHandler<WalletSweeper, WalletSweeperError> completion);

    void connect();

    void disconnect();

    void sync();

    void syncToDepth(WalletManagerSyncDepth depth);

    void submit(Transfer transfer, byte[] phraseUtf8);

    boolean isActive();

    System getSystem();

    Account getAccount();

    Network getNetwork();

    Wallet getPrimaryWallet();

    List<? extends Wallet> getWallets();

    WalletManagerMode getMode();

    void setMode(WalletManagerMode mode);

    String getPath();

    Currency getCurrency();

    String getName();

    Unit getBaseUnit();

    Unit getDefaultUnit();

    NetworkFee getDefaultNetworkFee();

    WalletManagerState getState();

    void setAddressScheme(AddressScheme scheme);

    AddressScheme getAddressScheme();
}
