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
    public CoreBRCryptoAmount getBalance() {
        return new OwnedBRCryptoAmount(CryptoLibrary.INSTANCE.cryptoWalletGetBalance(this));
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
    public CoreBRCryptoCurrency getCurrency() {
        return new OwnedBRCryptoCurrency(CryptoLibrary.INSTANCE.cryptoWalletGetCurrency(this));
    }

    @Override
    public CoreBRCryptoUnit getUnitForFee() {
        return new OwnedBRCryptoUnit(CryptoLibrary.INSTANCE.cryptoWalletGetUnitForFee(this));
    }

    @Override
    public CoreBRCryptoUnit getUnit() {
        return new OwnedBRCryptoUnit(CryptoLibrary.INSTANCE.cryptoWalletGetUnit(this));
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
        return new OwnedBRCryptoFeeBasis(CryptoLibrary.INSTANCE.cryptoWalletGetDefaultFeeBasis(this));
    }

    @Override
    public void setDefaultFeeBasis(CoreBRCryptoFeeBasis feeBasis) {
        CryptoLibrary.INSTANCE.cryptoWalletSetDefaultFeeBasis(this, feeBasis.asBRCryptoFeeBasis());
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
    public CoreBRCryptoTransfer createTransfer(BRCryptoAddress target, CoreBRCryptoAmount amount,
                                               CoreBRCryptoFeeBasis estimatedFeeBasis) {
        // TODO(discuss): This could return NULL, should be optional?
        return new OwnedBRCryptoTransfer(CryptoLibrary.INSTANCE.cryptoWalletCreateTransfer(this,
                target, amount.asBRCryptoAmount(), estimatedFeeBasis.asBRCryptoFeeBasis()));
    }

    @Override
    public Optional<CoreBRCryptoTransfer> createTransferForWalletSweep(BRCryptoWalletSweeper sweeper, CoreBRCryptoFeeBasis estimatedFeeBasis) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoWalletCreateTransferForWalletSweep(this,
                sweeper, estimatedFeeBasis.asBRCryptoFeeBasis())).transform(OwnedBRCryptoTransfer::new);
    }

    @Override
    public void estimateFeeBasis(Pointer cookie,
                                 BRCryptoAddress target, CoreBRCryptoAmount amount, CoreBRCryptoNetworkFee fee) {
        CryptoLibrary.INSTANCE.cryptoWalletEstimateFeeBasis(
                this,
                cookie,
                target,
                amount.asBRCryptoAmount(),
                fee.asBRCryptoNetworkFee());
    }

    @Override
    public void estimateFeeBasisForWalletSweep(Pointer cookie, BRCryptoWalletSweeper sweeper,
                                               CoreBRCryptoNetworkFee fee) {
        CryptoLibrary.INSTANCE.cryptoWalletEstimateFeeBasisForWalletSweep(
                this,
                cookie,
                sweeper,
                fee.asBRCryptoNetworkFee());
    }

    @Override
    public BRCryptoWallet asBRCryptoWallet() {
        return this;
    }
}
