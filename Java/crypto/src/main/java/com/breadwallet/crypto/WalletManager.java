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

import android.support.annotation.Nullable;

import com.breadwallet.crypto.errors.WalletSweeperError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;

import java.util.List;

public interface WalletManager {

    void createSweeper(Wallet wallet, Key key, CompletionHandler<WalletSweeper, WalletSweeperError> completion);

    void connect(@Nullable NetworkPeer peer);

    void disconnect();

    void sync();

    void stop();

    void syncToDepth(WalletManagerSyncDepth depth);

    void submit(Transfer transfer, byte[] phraseUtf8);

    boolean isActive();

    System getSystem();

    Account getAccount();

    Network getNetwork();

    Wallet getPrimaryWallet();

    List<? extends Wallet> getWallets();

    /**
     * Ensure that a wallet for currency exists.  If the wallet already exists, it is returned.
     * If the wallet needs to be created then `nil` is returned and a series of events will
     * occur - notably WalletEvent.created and WalletManagerEvent.walletAdded if the wallet is
     * created
     *
     * Note: There is a precondition on `currency` being one in the managers' network
     *
     * @return The wallet for currency if it already exists, otherwise "absent"
     */
    Optional<? extends Wallet> registerWalletFor(Currency currency);

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
