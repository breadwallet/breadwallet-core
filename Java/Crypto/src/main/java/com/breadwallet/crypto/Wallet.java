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
