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
import com.sun.jna.PointerType;

public class BRCryptoTransfer extends PointerType implements CoreBRCryptoTransfer {

    public BRCryptoTransfer(Pointer address) {
        super(address);
    }

    public BRCryptoTransfer() {
        super();
    }

    @Override
    public Optional<BRCryptoAddress> getSourceAddress() {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoTransferGetSourceAddress(this));
    }

    @Override
    public Optional<BRCryptoAddress> getTargetAddress() {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoTransferGetTargetAddress(this));
    }

    @Override
    public CoreBRCryptoAmount getAmount() {
        return new OwnedBRCryptoAmount(CryptoLibrary.INSTANCE.cryptoTransferGetAmount(this));
    }

    @Override
    public CoreBRCryptoAmount getAmountDirected() {
        return new OwnedBRCryptoAmount(CryptoLibrary.INSTANCE.cryptoTransferGetAmountDirected(this));
    }

    @Override
    public Optional<CoreBRCryptoHash> getHash() {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoTransferGetHash(this)).transform(OwnedBRCryptoHash::new);
    }

    @Override
    public int getDirection() {
        return CryptoLibrary.INSTANCE.cryptoTransferGetDirection(this);
    }

    @Override
    public BRCryptoTransferState getState() {
        return CryptoLibrary.INSTANCE.cryptoTransferGetState(this);
    }

    @Override
    public Optional<BRCryptoFeeBasis> getEstimatedFeeBasis() {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoTransferGetEstimatedFeeBasis(this));
    }

    @Override
    public Optional<BRCryptoFeeBasis> getConfirmedFeeBasis() {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoTransferGetConfirmedFeeBasis(this));
    }

    @Override
    public CoreBRCryptoUnit getUnitForFee() {
        return CryptoLibrary.INSTANCE.cryptoTransferGetUnitForFee(this);
    }

    @Override
    public CoreBRCryptoUnit getUnitForAmount() {
        return CryptoLibrary.INSTANCE.cryptoTransferGetUnitForAmount(this);
    }

    @Override
    public boolean isIdentical(CoreBRCryptoTransfer other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoTransferEqual(this, other.asBRCryptoTransfer());
    }

    @Override
    public BRCryptoTransfer asBRCryptoTransfer() {
        return this;
    }
}
