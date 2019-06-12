/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.primitives.UnsignedLong;

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
    public CoreBRCryptoAmount getBalance() {
        return core.getBalance();
    }

    @Override
    public UnsignedLong getTransferCount() {
        return core.getTransferCount();
    }

    @Override
    public CoreBRCryptoTransfer getTransfer(UnsignedLong index) {
        return core.getTransfer(index);
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
    public CoreBRCryptoFeeBasis getDefaultFeeBasis() {
        return core.getDefaultFeeBasis();
    }

    @Override
    public void setDefaultFeeBasis(CoreBRCryptoFeeBasis feeBasis) {
        core.setDefaultFeeBasis(feeBasis);
    }

    @Override
    public CoreBRCryptoAddress getSourceAddress() {
        return core.getSourceAddress();
    }

    @Override
    public CoreBRCryptoAddress getTargetAddress() {
        return core.getTargetAddress();
    }

    @Override
    public CoreBRCryptoTransfer createTransfer(CoreBRCryptoAddress target, CoreBRCryptoAmount amount, CoreBRCryptoFeeBasis feeBasis) {
        return core.createTransfer(target, amount, feeBasis);
    }

    @Override
    public CoreBRCryptoAmount estimateFee(CoreBRCryptoAmount amount, CoreBRCryptoFeeBasis feeBasis, CoreBRCryptoUnit unit) {
        return core.estimateFee(amount, feeBasis, unit);
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
