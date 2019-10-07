/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.google.common.base.Optional;

public interface CoreBRCryptoTransfer {

    static CoreBRCryptoTransfer createOwned(BRCryptoTransfer transfer) {
        return new OwnedBRCryptoTransfer(transfer);
    }

    Optional<BRCryptoAddress> getSourceAddress();

    Optional<BRCryptoAddress> getTargetAddress();

    CoreBRCryptoAmount getAmount();

    CoreBRCryptoAmount getAmountDirected();

    Optional<CoreBRCryptoHash> getHash();

    int getDirection();

    BRCryptoTransferState getState();

    Optional<BRCryptoFeeBasis> getEstimatedFeeBasis();

    Optional<BRCryptoFeeBasis> getConfirmedFeeBasis();

    CoreBRCryptoUnit getUnitForFee();
    
    CoreBRCryptoUnit getUnitForAmount();

    boolean isIdentical(CoreBRCryptoTransfer other);

    BRCryptoTransfer asBRCryptoTransfer();
}
