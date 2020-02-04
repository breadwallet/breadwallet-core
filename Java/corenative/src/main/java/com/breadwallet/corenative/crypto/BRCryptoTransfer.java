/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoTransfer extends PointerType {

    public BRCryptoTransfer() {
        super();
    }

    public BRCryptoTransfer(Pointer address) {
        super(address);
    }

    public Optional<BRCryptoAddress> getSourceAddress() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoTransferGetSourceAddress(
                        thisPtr
                )
        ).transform(BRCryptoAddress::new);
    }

    public Optional<BRCryptoAddress> getTargetAddress() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoTransferGetTargetAddress(
                        thisPtr
                )
        ).transform(BRCryptoAddress::new);
    }

    public BRCryptoAmount getAmount() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoAmount(CryptoLibraryDirect.cryptoTransferGetAmount(thisPtr));
    }

    public BRCryptoAmount getAmountDirected() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoAmount(CryptoLibraryDirect.cryptoTransferGetAmountDirected(thisPtr));
    }

    public Optional<BRCryptoHash> getHash() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoTransferGetHash(
                        thisPtr
                )
        ).transform(BRCryptoHash::new);
    }

    public BRCryptoTransferDirection getDirection() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoTransferDirection.fromCore(CryptoLibraryDirect.cryptoTransferGetDirection(thisPtr));
    }

    public BRCryptoTransferState getState() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoTransferGetState(thisPtr);
    }

    public Optional<BRCryptoFeeBasis> getEstimatedFeeBasis() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoTransferGetEstimatedFeeBasis(
                        thisPtr
                )
        ).transform(BRCryptoFeeBasis::new);
    }

    public Optional<BRCryptoFeeBasis> getConfirmedFeeBasis() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoTransferGetConfirmedFeeBasis(
                        thisPtr
                )
        ).transform(BRCryptoFeeBasis::new);
    }

    public UnsignedLong getAttributeCount() {
        Pointer thisPtr = this.getPointer();

        return UnsignedLong.fromLongBits(
                CryptoLibraryDirect.cryptoTransferGetAttributeCount(
                        thisPtr
                ).longValue()
        );
    }

    public Optional<BRCryptoTransferAttribute> getAttributeAt(UnsignedLong index) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoTransferGetAttributeAt(
                        thisPtr,
                        new SizeT(index.longValue())
                )
        ).transform(BRCryptoTransferAttribute::new);
    }

    public BRCryptoUnit getUnitForFee() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoUnit(CryptoLibraryDirect.cryptoTransferGetUnitForFee(thisPtr));
    }

    public BRCryptoUnit getUnitForAmount() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoUnit(CryptoLibraryDirect.cryptoTransferGetUnitForAmount(thisPtr));
    }

    public boolean isIdentical(BRCryptoTransfer other) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoTransferEqual(thisPtr, other.getPointer());
    }

    public BRCryptoTransfer take() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoTransfer(CryptoLibraryDirect.cryptoTransferTake(thisPtr));
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoTransferGive(thisPtr);
    }
}
