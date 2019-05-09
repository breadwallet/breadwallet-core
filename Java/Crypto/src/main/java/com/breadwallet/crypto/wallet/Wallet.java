/*
 * Wallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.wallet;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Currency;
import com.breadwallet.crypto.transfer.Transfer;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.transfer.TransferFeeBasis;
import com.breadwallet.crypto.transfer.TransferHash;
import com.breadwallet.crypto.walletmanager.WalletManager;
import com.google.common.base.Optional;

import java.util.List;

public interface Wallet {

    WalletManager getWalletManager();

    Unit getBaseUnit();

    Amount getBalance();

    List<Transfer> getTransfers();

    Optional<Transfer> getTransferByHash(TransferHash hash);

    WalletState getState();

    TransferFeeBasis getDefaultFeeBasis();

    void getDefaultFeeBasis(TransferFeeBasis feeBasis);

    Address getTarget();

    Address getSource();

    Optional<Transfer> createTransfer(Address target, Amount amount, TransferFeeBasis feeBasis);

    Amount estimateFee(Amount amount, TransferFeeBasis feeBasis);

    default Optional<Transfer> createTransfer(Address target, Amount amount) {
        return createTransfer(target, amount, getDefaultFeeBasis());
    }

    default Currency getCurrency() {
        return getBaseUnit().getCurrency();
    }


    default String getName() {
        return getBaseUnit().getCurrency().getCode();
    }

}
