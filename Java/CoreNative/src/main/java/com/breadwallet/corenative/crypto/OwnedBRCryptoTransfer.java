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

import java.util.Objects;

/* package */
class OwnedBRCryptoTransfer implements CoreBRCryptoTransfer {

    private final BRCryptoTransfer core;

    /* package */
    OwnedBRCryptoTransfer(BRCryptoTransfer core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoTransferGive(core);
        }
    }

    @Override
    public Optional<CoreBRCryptoAddress> getSourceAddress() {
        return core.getSourceAddress();
    }

    @Override
    public Optional<CoreBRCryptoAddress> getTargetAddress() {
        return core.getTargetAddress();
    }

    @Override
    public CoreBRCryptoAmount getAmount() {
        return core.getAmount();
    }

    @Override
    public CoreBRCryptoAmount getFee() {
        return core.getFee();
    }

    @Override
    public CoreBRCryptoFeeBasis getFeeBasis() {
        return core.getFeeBasis();
    }

    @Override
    public Optional<CoreBRCryptoHash> getHash() {
        return core.getHash();
    }

    @Override
    public int getDirection() {
        return core.getDirection();
    }

    @Override
    public boolean isIdentical(CoreBRCryptoTransfer other) {
        return core.isIdentical(other);
    }

    @Override
    public BRCryptoTransfer asBRCryptoTransfer() {
        return core;
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoTransfer) {
            OwnedBRCryptoTransfer that = (OwnedBRCryptoTransfer) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoTransfer) {
            BRCryptoTransfer that = (BRCryptoTransfer) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
