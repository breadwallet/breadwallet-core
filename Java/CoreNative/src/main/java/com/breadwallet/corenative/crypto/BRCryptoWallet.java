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

public class BRCryptoWallet extends PointerType {

    public static BRCryptoWallet createOwned(BRCryptoWallet wallet) {
        // TODO(fix): Can the use case here (called when parsed out of struct) be replaced by changing struct to
        //            have BRCryptoWallet.OwnedBRCryptoWallet as its field, instead of BRCryptoWallet?
        return new OwnedBRCryptoWallet(wallet.getPointer());
    }

    public BRCryptoWallet(Pointer address) {
        super(address);
    }

    public BRCryptoWallet() {
        super();
    }

    public BRCryptoAmount getBalance() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetBalance(this);
    }

    public List<BRCryptoTransfer> getTransfers() {
        List<BRCryptoTransfer> transfers = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer transfersPtr = CryptoLibrary.INSTANCE.cryptoWalletGetTransfers(this, count);
        if (null != transfersPtr) {
            try {
                int transfersSize = UnsignedInts.checkedCast(count.getValue().longValue());
                for (Pointer transferPtr: transfersPtr.getPointerArray(0, transfersSize)) {
                    transfers.add(new BRCryptoTransfer.OwnedBRCryptoTransfer(transferPtr));
                }

            } finally {
                Native.free(Pointer.nativeValue(transfersPtr));
            }
        }
        return transfers;
    }


    public boolean containsTransfer(BRCryptoTransfer transfer) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoWalletHasTransfer(this,
                transfer);
    }

    public BRCryptoCurrency getCurrency() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetCurrency(this);
    }

    public BRCryptoUnit getUnitForFee() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetUnitForFee(this);
    }

    public BRCryptoUnit getUnit() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetUnit(this);
    }

    public int getState() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetState(this);
    }

    public void setState(int state) {
        CryptoLibrary.INSTANCE.cryptoWalletSetState(this, state);
    }

    public BRCryptoFeeBasis getDefaultFeeBasis() {
        return CryptoLibrary.INSTANCE.cryptoWalletGetDefaultFeeBasis(this);
    }

    public void setDefaultFeeBasis(BRCryptoFeeBasis feeBasis) {
        CryptoLibrary.INSTANCE.cryptoWalletSetDefaultFeeBasis(this, feeBasis);
    }

    public BRCryptoAddress getSourceAddress(int addressScheme) {
        return CryptoLibrary.INSTANCE.cryptoWalletGetAddress(this, addressScheme);
    }

    public BRCryptoAddress getTargetAddress(int addressScheme) {
        return CryptoLibrary.INSTANCE.cryptoWalletGetAddress(this, addressScheme);
    }

    public BRCryptoTransfer createTransfer(BRCryptoAddress target, BRCryptoAmount amount,
                                               BRCryptoFeeBasis estimatedFeeBasis) {
        // TODO(discuss): This could return NULL, should be optional?
        return CryptoLibrary.INSTANCE.cryptoWalletCreateTransfer(this,
                target, amount, estimatedFeeBasis);
    }

    public Optional<BRCryptoTransfer> createTransferForWalletSweep(BRCryptoWalletSweeper sweeper, BRCryptoFeeBasis estimatedFeeBasis) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoWalletCreateTransferForWalletSweep(this,
                sweeper, estimatedFeeBasis));
    }

    public void estimateFeeBasis(Pointer cookie,
                                 BRCryptoAddress target, BRCryptoAmount amount, BRCryptoNetworkFee fee) {
        CryptoLibrary.INSTANCE.cryptoWalletEstimateFeeBasis(
                this,
                cookie,
                target,
                amount,
                fee);
    }

    public void estimateFeeBasisForWalletSweep(Pointer cookie, BRCryptoWalletSweeper sweeper,
                                               BRCryptoNetworkFee fee) {
        CryptoLibrary.INSTANCE.cryptoWalletEstimateFeeBasisForWalletSweep(
                this,
                cookie,
                sweeper,
                fee);
    }

    public static class OwnedBRCryptoWallet extends BRCryptoWallet {

        public OwnedBRCryptoWallet(Pointer address) {
            super(address);
        }

        public OwnedBRCryptoWallet() {
            super();
        }

        @Override
        protected void finalize() {
            if (null != getPointer()) {
                CryptoLibrary.INSTANCE.cryptoWalletGive(this);
            }
        }
    }
}
