/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
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
    public Optional<CoreBRCryptoAddress> getSourceAddress() {
        BRCryptoAddress address = CryptoLibrary.INSTANCE.cryptoTransferGetSourceAddress(this);
        if (null != address) {
            address = CryptoLibrary.INSTANCE.cryptoAddressTake(address);
        }
        return Optional.fromNullable(address).transform(OwnedBRCryptoAddress::new);
    }

    @Override
    public Optional<CoreBRCryptoAddress> getTargetAddress() {
        BRCryptoAddress address = CryptoLibrary.INSTANCE.cryptoTransferGetTargetAddress(this);
        if (null != address) {
            address = CryptoLibrary.INSTANCE.cryptoAddressTake(address);
        }
        return Optional.fromNullable(address).transform(OwnedBRCryptoAddress::new);
    }

    @Override
    public CoreBRCryptoAmount getAmount() {
        return new OwnedBRCryptoAmount(CryptoLibrary.INSTANCE.cryptoTransferGetAmount(this));
    }

    @Override
    public CoreBRCryptoAmount getFee() {
        return new OwnedBRCryptoAmount(CryptoLibrary.INSTANCE.cryptoTransferGetFee(this));
    }

    @Override
    public CoreBRCryptoFeeBasis getFeeBasis() {
        return new OwnedBRCryptoFeeBasis(CryptoLibrary.INSTANCE.cryptoTransferGetFeeBasis(this));
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
    public boolean isIdentical(CoreBRCryptoTransfer other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoTransferEqual(this, other.asBRCryptoTransfer());
    }

    @Override
    public BRCryptoTransfer asBRCryptoTransfer() {
        return this;
    }
}
