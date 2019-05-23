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
import com.sun.jna.Pointer;

import java.util.List;

public abstract class Wallet {

    /* package */
    abstract Pointer getPointer();

    public abstract Unit getBaseUnit();

    public abstract Amount getBalance();

    public abstract List<Transfer> getTransfers();

    public abstract Optional<Transfer> getTransferByHash(TransferHash hash);

    public abstract WalletState getState();

    public abstract TransferFeeBasis getDefaultFeeBasis();

    public abstract void setDefaultFeeBasis(TransferFeeBasis feeBasis);

    public abstract Address getTarget();

    public abstract Address getSource();

    public abstract Optional<Transfer> createTransfer(Address target, Amount amount, TransferFeeBasis feeBasis);

    public abstract Amount estimateFee(Amount amount, TransferFeeBasis feeBasis);

    public Optional<Transfer> createTransfer(Address target, Amount amount) {
        return createTransfer(target, amount, getDefaultFeeBasis());
    }

    public Currency getCurrency() {
        return getBaseUnit().getCurrency();
    }

    public String getName() {
        return getBaseUnit().getCurrency().getCode();
    }
}
