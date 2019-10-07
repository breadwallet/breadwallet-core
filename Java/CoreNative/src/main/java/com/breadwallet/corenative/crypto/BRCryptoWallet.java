/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInts;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import java.util.ArrayList;
import java.util.List;

public class BRCryptoWallet extends PointerType implements CoreBRCryptoWallet {

    public BRCryptoWallet(Pointer address) {
        super(address);
    }

    public BRCryptoWallet() {
        super();
    }

    @Override
    public BRCryptoAmount getBalance() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetBalance(this);
    }

    @Override
    public List<CoreBRCryptoTransfer> getTransfers() {
        List<CoreBRCryptoTransfer> transfers = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer transfersPtr = CryptoLibrary.INSTANCE.cryptoWalletGetTransfers(this, count);
        if (null != transfersPtr) {
            try {
                int transfersSize = UnsignedInts.checkedCast(count.getValue().longValue());
                for (Pointer transferPtr: transfersPtr.getPointerArray(0, transfersSize)) {
                    transfers.add(new OwnedBRCryptoTransfer(new BRCryptoTransfer(transferPtr)));
                }

            } finally {
                Native.free(Pointer.nativeValue(transfersPtr));
            }
        }
        return transfers;
    }


    public boolean containsTransfer(CoreBRCryptoTransfer transfer) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoWalletHasTransfer(this,
                transfer.asBRCryptoTransfer());
    }

    @Override
    public BRCryptoCurrency getCurrency() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetCurrency(this);
    }

    @Override
    public BRCryptoUnit getUnitForFee() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetUnitForFee(this);
    }

    @Override
    public BRCryptoUnit getUnit() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetUnit(this);
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
    public BRCryptoFeeBasis getDefaultFeeBasis() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetDefaultFeeBasis(this);
    }

    @Override
    public void setDefaultFeeBasis(BRCryptoFeeBasis feeBasis) {
        CryptoLibrary.INSTANCE.cryptoWalletSetDefaultFeeBasis(this, feeBasis);
    }

    @Override
    public BRCryptoAddress getSourceAddress(int addressScheme) {
        return CryptoLibrary.INSTANCE.cryptoWalletGetAddress(this, addressScheme);
    }

    @Override
    public BRCryptoAddress getTargetAddress(int addressScheme) {
        return CryptoLibrary.INSTANCE.cryptoWalletGetAddress(this, addressScheme);
    }

    @Override
    public CoreBRCryptoTransfer createTransfer(BRCryptoAddress target, BRCryptoAmount amount,
                                               BRCryptoFeeBasis estimatedFeeBasis) {
        // TODO(discuss): This could return NULL, should be optional?
        return new OwnedBRCryptoTransfer(CryptoLibrary.INSTANCE.cryptoWalletCreateTransfer(this,
                target, amount, estimatedFeeBasis));
    }

    @Override
    public Optional<CoreBRCryptoTransfer> createTransferForWalletSweep(BRCryptoWalletSweeper sweeper, BRCryptoFeeBasis estimatedFeeBasis) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoWalletCreateTransferForWalletSweep(this,
                sweeper, estimatedFeeBasis)).transform(OwnedBRCryptoTransfer::new);
    }

    @Override
    public void estimateFeeBasis(Pointer cookie,
                                 BRCryptoAddress target, BRCryptoAmount amount, BRCryptoNetworkFee fee) {
        CryptoLibrary.INSTANCE.cryptoWalletEstimateFeeBasis(
                this,
                cookie,
                target,
                amount,
                fee);
    }

    @Override
    public void estimateFeeBasisForWalletSweep(Pointer cookie, BRCryptoWalletSweeper sweeper,
                                               BRCryptoNetworkFee fee) {
        CryptoLibrary.INSTANCE.cryptoWalletEstimateFeeBasisForWalletSweep(
                this,
                cookie,
                sweeper,
                fee);
    }

    @Override
    public BRCryptoWallet asBRCryptoWallet() {
        return this;
    }
}
