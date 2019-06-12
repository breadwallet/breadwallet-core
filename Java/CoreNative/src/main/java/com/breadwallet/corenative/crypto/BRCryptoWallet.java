/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoWallet extends PointerType implements CoreBRCryptoWallet {

    public BRCryptoWallet(Pointer address) {
        super(address);
    }

    public BRCryptoWallet() {
        super();
    }

    @Override
    public CoreBRCryptoAmount getBalance() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetBalance(this);
    }

    @Override
    public UnsignedLong getTransferCount() {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.cryptoWalletGetTransferCount(this).longValue());
    }

    @Override
    public CoreBRCryptoTransfer getTransfer(UnsignedLong index) {
        BRCryptoTransfer transfer = CryptoLibrary.INSTANCE.cryptoWalletGetTransfer(this, new SizeT(index.longValue()));
        transfer = CryptoLibrary.INSTANCE.cryptoTransferTake(transfer);
        return new OwnedBRCryptoTransfer(transfer);
    }

    @Override
    public int getState() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetState(this);
    }

    @Override
    public void setState(int state) {
        CryptoLibrary.INSTANCE.cryptoWalletSetState(this, state);
    }

    @Override
    public CoreBRCryptoFeeBasis getDefaultFeeBasis() {
        // TODO(discuss): This could return NULL, should be optional?
        return CryptoLibrary.INSTANCE.cryptoWalletGetDefaultFeeBasis(this);
    }

    @Override
    public void setDefaultFeeBasis(CoreBRCryptoFeeBasis feeBasis) {
        CryptoLibrary.INSTANCE.cryptoWalletSetDefaultFeeBasis(this, feeBasis.asBRCryptoFeeBasis());
    }

    @Override
    public CoreBRCryptoAddress getSourceAddress() {
        // TODO(discuss): This could return NULL, should be optional?
        return CryptoLibrary.INSTANCE.cryptoWalletGetAddress(this);
    }

    @Override
    public CoreBRCryptoAddress getTargetAddress() {
        // TODO(discuss): This could return NULL, should be optional?
        return CryptoLibrary.INSTANCE.cryptoWalletGetAddress(this);
    }

    @Override
    public CoreBRCryptoTransfer createTransfer(CoreBRCryptoAddress target, CoreBRCryptoAmount amount, CoreBRCryptoFeeBasis feeBasis) {
        // TODO(discuss): This could return NULL, should be optional?
        return CryptoLibrary.INSTANCE.cryptoWalletCreateTransfer(this, target.asBRCryptoAddress(), amount.asBRCryptoAmount(), feeBasis.asBRCryptoFeeBasis());
    }

    @Override
    public CoreBRCryptoAmount estimateFee(CoreBRCryptoAmount amount, CoreBRCryptoFeeBasis feeBasis, CoreBRCryptoUnit unit) {
        // TODO(discuss): This could return NULL, should be optional?
        return CryptoLibrary.INSTANCE.cryptoWalletEstimateFee(this, amount.asBRCryptoAmount(), feeBasis.asBRCryptoFeeBasis(), unit.asBRCryptoUnit());
    }

    @Override
    public BRCryptoWallet asBRCryptoWallet() {
        return this;
    }
}
