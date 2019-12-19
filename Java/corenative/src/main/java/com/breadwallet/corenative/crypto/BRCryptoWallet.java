/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.utility.Cookie;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInts;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.ptr.IntByReference;

import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

public class BRCryptoWallet extends PointerType {

    public BRCryptoWallet() {
        super();
    }

    public BRCryptoWallet(Pointer address) {
        super(address);
    }

    public BRCryptoAmount getBalance() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoAmount(CryptoLibraryDirect.cryptoWalletGetBalance(thisPtr));
    }

    public List<BRCryptoTransfer> getTransfers() {
        Pointer thisPtr = this.getPointer();

        List<BRCryptoTransfer> transfers = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer transfersPtr = CryptoLibraryDirect.cryptoWalletGetTransfers(thisPtr, count);
        if (null != transfersPtr) {
            try {
                int transfersSize = UnsignedInts.checkedCast(count.getValue().longValue());
                for (Pointer transferPtr: transfersPtr.getPointerArray(0, transfersSize)) {
                    transfers.add(new BRCryptoTransfer(transferPtr));
                }

            } finally {
                Native.free(Pointer.nativeValue(transfersPtr));
            }
        }
        return transfers;
    }


    public boolean containsTransfer(BRCryptoTransfer transfer) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoWalletHasTransfer(thisPtr, transfer.getPointer());
    }

    public BRCryptoCurrency getCurrency() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoCurrency(CryptoLibraryDirect.cryptoWalletGetCurrency(thisPtr));
    }

    public BRCryptoUnit getUnitForFee() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoUnit(CryptoLibraryDirect.cryptoWalletGetUnitForFee(thisPtr));
    }

    public BRCryptoUnit getUnit() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoUnit(CryptoLibraryDirect.cryptoWalletGetUnit(thisPtr));
    }

    public BRCryptoWalletState getState() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoWalletState.fromCore(CryptoLibraryDirect.cryptoWalletGetState(thisPtr));
    }

    public BRCryptoAddress getSourceAddress(BRCryptoAddressScheme addressScheme) {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoAddress(CryptoLibraryDirect.cryptoWalletGetAddress(thisPtr, addressScheme.toCore()));
    }

    public BRCryptoAddress getTargetAddress(BRCryptoAddressScheme addressScheme) {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoAddress(CryptoLibraryDirect.cryptoWalletGetAddress(thisPtr, addressScheme.toCore()));
    }

    public Optional<BRCryptoFeeBasis> createTransferFeeBasis(BRCryptoAmount pricePerCostFactor, double costFactor) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoWalletCreateFeeBasis(
                        thisPtr,
                        pricePerCostFactor.getPointer(),
                        costFactor
                )
        ).transform(BRCryptoFeeBasis::new);
    }

    public Optional<BRCryptoTransfer> createTransfer(BRCryptoAddress target, BRCryptoAmount amount,
                                                     BRCryptoFeeBasis estimatedFeeBasis) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoWalletCreateTransfer(
                        thisPtr,
                        target.getPointer(),
                        amount.getPointer(),
                        estimatedFeeBasis.getPointer()
                )
        ).transform(BRCryptoTransfer::new);
    }

    public Optional<BRCryptoTransfer> createTransferForWalletSweep(BRCryptoWalletSweeper sweeper, BRCryptoFeeBasis estimatedFeeBasis) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoWalletCreateTransferForWalletSweep(
                        thisPtr,
                        sweeper.getPointer(),
                        estimatedFeeBasis.getPointer()
                )
        ).transform(BRCryptoTransfer::new);
    }

    public Optional<BRCryptoTransfer> createTransferForPaymentProtocolRequest(BRCryptoPaymentProtocolRequest request, BRCryptoFeeBasis estimatedFeeBasis) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoWalletCreateTransferForPaymentProtocolRequest(
                        thisPtr,
                        request.getPointer(),
                        estimatedFeeBasis.getPointer()
                )
        ).transform(BRCryptoTransfer::new);
    }

    public void estimateFeeBasis(Cookie cookie,
                                 BRCryptoAddress target, BRCryptoAmount amount, BRCryptoNetworkFee fee) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletEstimateFeeBasis(
                thisPtr,
                cookie.getPointer(),
                target.getPointer(),
                amount.getPointer(),
                fee.getPointer());
    }

    public void estimateFeeBasisForWalletSweep(Cookie cookie, BRCryptoWalletSweeper sweeper,
                                               BRCryptoNetworkFee fee) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletEstimateFeeBasisForWalletSweep(
                thisPtr,
                cookie.getPointer(),
                sweeper.getPointer(),
                fee.getPointer());
    }

    public void estimateFeeBasisForPaymentProtocolRequest(Cookie cookie, BRCryptoPaymentProtocolRequest request,
                                                          BRCryptoNetworkFee fee) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletEstimateFeeBasisForPaymentProtocolRequest(
                thisPtr,
                cookie.getPointer(),
                request.getPointer(),
                fee.getPointer());
    }

    public static class EstimateLimitResult {

        public @Nullable BRCryptoAmount amount;
        public boolean needFeeEstimate;
        public boolean isZeroIfInsuffientFunds;

        EstimateLimitResult(@Nullable BRCryptoAmount amount, boolean needFeeEstimate, boolean isZeroIfInsuffientFunds) {
            this.amount = amount;
            this.needFeeEstimate = needFeeEstimate;
            this.isZeroIfInsuffientFunds = isZeroIfInsuffientFunds;
        }
    }

    public EstimateLimitResult estimateLimit(boolean asMaximum, BRCryptoAddress coreAddress, BRCryptoNetworkFee coreFee) {
        Pointer thisPtr = this.getPointer();

        IntByReference needFeeEstimateRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        IntByReference isZeroIfInsuffientFundsRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        Optional<BRCryptoAmount> maybeAmount = Optional.fromNullable(CryptoLibraryDirect.cryptoWalletEstimateLimit(
                thisPtr,
                asMaximum ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE,
                coreAddress.getPointer(),
                coreFee.getPointer(),
                needFeeEstimateRef,
                isZeroIfInsuffientFundsRef
        )).transform(BRCryptoAmount::new);

        return new EstimateLimitResult(
                maybeAmount.orNull(),
                needFeeEstimateRef.getValue() == BRCryptoBoolean.CRYPTO_TRUE,
                isZeroIfInsuffientFundsRef.getValue() == BRCryptoBoolean.CRYPTO_TRUE
        );
    }

    public BRCryptoWallet take() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoWallet(CryptoLibraryDirect.cryptoWalletTake(thisPtr));
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletGive(thisPtr);
    }
}
