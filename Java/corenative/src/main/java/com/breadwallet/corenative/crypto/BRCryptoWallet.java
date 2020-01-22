/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.CryptoLibraryIndirect;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
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

    public Optional<BRCryptoAmount> getBalanceMaximum () {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoWalletGetBalanceMaximum(thisPtr)
        ).transform(BRCryptoAmount::new);
    }

    public Optional<BRCryptoAmount> getBalanceMinimum () {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoWalletGetBalanceMinimum(thisPtr)
        ).transform(BRCryptoAmount::new);
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

    public UnsignedLong getTransferAttributeCount(@Nullable BRCryptoAddress target) {
        Pointer thisPtr = this.getPointer();
        Pointer targetPtr = (null == target ? null : target.getPointer());

        return UnsignedLong.fromLongBits(
                CryptoLibraryDirect.cryptoWalletGetTransferAttributeCount(
                        thisPtr,
                        targetPtr
                ).longValue()
        );
    }

    public Optional<BRCryptoTransferAttribute> getTransferAttributeAt(@Nullable BRCryptoAddress target, UnsignedLong index) {
        Pointer thisPtr = this.getPointer();
        Pointer targetPtr = (null == target ? null : target.getPointer());

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoWalletGetTransferAttributeAt(
                        thisPtr,
                        targetPtr,
                        new SizeT(index.longValue())
                )
        ).transform(BRCryptoTransferAttribute::new);
    }

    public Optional<BRCryptoTransferAttributeValidationError> validateTransferAttribute(BRCryptoTransferAttribute attribute) {
        Pointer thisPtr = this.getPointer();

        IntByReference validates = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);

        BRCryptoTransferAttributeValidationError error = BRCryptoTransferAttributeValidationError.fromCore(
                CryptoLibraryDirect.cryptoWalletValidateTransferAttribute(
                        thisPtr,
                        attribute.getPointer(),
                        validates
                ));

        return (BRCryptoBoolean.CRYPTO_TRUE == validates.getValue()
                ? Optional.absent()
                : Optional.of(error));
    }

    public Optional<BRCryptoTransferAttributeValidationError> validateTransferAttributes(List<BRCryptoTransferAttribute> attributes) {
        Pointer thisPtr = this.getPointer();

        IntByReference validates = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);

        int attributesCount  = attributes.size();
        BRCryptoTransferAttribute[] attributeRefs = new BRCryptoTransferAttribute[attributesCount];
        for (int i = 0; i < attributesCount; i++) attributeRefs[i] = attributes.get(i);

        BRCryptoTransferAttributeValidationError error = BRCryptoTransferAttributeValidationError.fromCore(
                CryptoLibraryIndirect.cryptoWalletValidateTransferAttributes(
                        thisPtr,
                        new SizeT(attributes.size()),
                        attributeRefs,
                        validates
                ));

        return (BRCryptoBoolean.CRYPTO_TRUE == validates.getValue()
                ? Optional.absent()
                : Optional.of(error));
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

    public BRCryptoAddress getTargetAddress(BRCryptoAddressScheme addressScheme) {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoAddress(CryptoLibraryDirect.cryptoWalletGetAddress(thisPtr, addressScheme.toCore()));
    }

    public boolean containsAddress(BRCryptoAddress address) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoWalletHasAddress(
                this.getPointer(),
                address.getPointer()
        );
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
                                                     BRCryptoFeeBasis estimatedFeeBasis,
                                                     List<BRCryptoTransferAttribute> attributes) {
        Pointer thisPtr = this.getPointer();

        int attributesCount  = attributes.size();
        BRCryptoTransferAttribute[] attributeRefs = new BRCryptoTransferAttribute[attributesCount];
        for (int i = 0; i < attributesCount; i++) attributeRefs[i] = attributes.get(i);

        return Optional.fromNullable(
                CryptoLibraryIndirect.cryptoWalletCreateTransfer(
                        thisPtr,
                        target.getPointer(),
                        amount.getPointer(),
                        estimatedFeeBasis.getPointer(),
                        new SizeT(attributesCount),
                        attributeRefs
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

     public BRCryptoWallet take() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoWallet(CryptoLibraryDirect.cryptoWalletTake(thisPtr));
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletGive(thisPtr);
    }
}
