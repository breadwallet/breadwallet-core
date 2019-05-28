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

import com.breadwallet.crypto.jni.bitcoin.BRTransaction;
import com.breadwallet.crypto.jni.bitcoin.BRWallet;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.util.List;

import javax.annotation.Nullable;

public abstract class Wallet {

    abstract boolean matches(BRWallet walletImpl);

    /* package */
    abstract Optional<Transfer> getOrCreateTransferByImpl(BRTransaction transferImpl, boolean createAllowed);

    /* package */
    abstract WalletState setState(WalletState state);

    /* package */
    abstract void setDefaultFeeBasis(TransferFeeBasis feeBasis);

    public abstract WalletManager getWalletManager();

    public abstract Unit getBaseUnit();

    public abstract Amount getBalance();

    public abstract List<Transfer> getTransfers();

    public abstract Optional<Transfer> getTransferByHash(TransferHash hash);

    public abstract WalletState getState();

    public abstract TransferFeeBasis getDefaultFeeBasis();

    public abstract Address getTarget();

    public abstract Address getSource();

    public abstract Optional<Transfer> createTransfer(Address target, Amount amount, TransferFeeBasis feeBasis);

    public abstract Amount estimateFee(Amount amount, @Nullable TransferFeeBasis feeBasis);

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
