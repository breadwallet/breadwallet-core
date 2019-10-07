/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.util.List;
import java.util.Objects;

/* package */
class OwnedBRCryptoWallet implements CoreBRCryptoWallet {

    private final BRCryptoWallet core;

    /* package */
    OwnedBRCryptoWallet(BRCryptoWallet core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoWalletGive(core);
        }
    }

    @Override
    public BRCryptoAmount getBalance() {
        return core.getBalance();
    }

    @Override
    public List<CoreBRCryptoTransfer> getTransfers() {
        return core.getTransfers();
    }

    @Override
    public boolean containsTransfer(CoreBRCryptoTransfer transfer) {
        return core.containsTransfer(transfer);
    }

    @Override
    public BRCryptoCurrency getCurrency() {
        return core.getCurrency();
    }

    @Override
    public CoreBRCryptoUnit getUnitForFee() {
        return core.getUnitForFee();
    }

    @Override
    public CoreBRCryptoUnit getUnit() {
        return core.getUnit();
    }

    @Override
    public int getState() {
        return core.getState();
    }

    @Override
    public void setState(int state) {
        core.setState(state);
    }

    @Override
    public BRCryptoFeeBasis getDefaultFeeBasis() {
        return core.getDefaultFeeBasis();
    }

    @Override
    public void setDefaultFeeBasis(BRCryptoFeeBasis feeBasis) {
        core.setDefaultFeeBasis(feeBasis);
    }

    @Override
    public BRCryptoAddress getSourceAddress(int addressScheme) {
        return core.getSourceAddress(addressScheme);
    }

    @Override
    public BRCryptoAddress getTargetAddress(int addressScheme) {
        return core.getTargetAddress(addressScheme);
    }

    @Override
    public CoreBRCryptoTransfer createTransfer(BRCryptoAddress target, BRCryptoAmount amount, BRCryptoFeeBasis estimatedFeeBasis) {
        return core.createTransfer(target, amount, estimatedFeeBasis);
    }

    @Override
    public Optional<CoreBRCryptoTransfer> createTransferForWalletSweep(BRCryptoWalletSweeper sweeper, BRCryptoFeeBasis estimatedFeeBasis) {
        return core.createTransferForWalletSweep(sweeper, estimatedFeeBasis);
    }

    @Override
    public void estimateFeeBasis(Pointer cookie, BRCryptoAddress target, BRCryptoAmount amount, BRCryptoNetworkFee fee) {
        core.estimateFeeBasis(cookie, target, amount, fee);
    }

    @Override
    public void estimateFeeBasisForWalletSweep(Pointer cookie, BRCryptoWalletSweeper sweeper,
                                               BRCryptoNetworkFee fee) {
        core.estimateFeeBasisForWalletSweep(cookie, sweeper, fee);
    }

    @Override
    public BRCryptoWallet asBRCryptoWallet() {
        return core;
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoWallet) {
            OwnedBRCryptoWallet that = (OwnedBRCryptoWallet) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoWallet) {
            BRCryptoWallet that = (BRCryptoWallet) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
