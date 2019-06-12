/*
 * Wallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;

import java.util.List;

public interface Wallet {

    Optional<? extends Transfer> createTransfer(Address target, Amount amount);

    Optional<? extends Transfer> createTransfer(Address target, Amount amount, TransferFeeBasis feeBasis);

    Amount estimateFee(Amount amount);

    Amount estimateFee(Amount amount, TransferFeeBasis feeBasis);

    WalletManager getWalletManager();

    Unit getBaseUnit();

    Amount getBalance();

    List<? extends Transfer> getTransfers();

    Optional<? extends Transfer> getTransferByHash(TransferHash hash);

    TransferFeeBasis getDefaultFeeBasis();

    Address getTarget();

    Address getSource();

    Currency getCurrency();

    String getName();

    WalletState getState();
}
