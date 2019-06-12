/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.google.common.base.Optional;

public interface CoreBRCryptoTransfer {

    static CoreBRCryptoTransfer create(BRCryptoTransfer transfer) {
        return new OwnedBRCryptoTransfer(transfer);
    }

    Optional<CoreBRCryptoAddress> getSourceAddress();

    Optional<CoreBRCryptoAddress> getTargetAddress();

    Optional<CoreBRCryptoAmount> getAmount();

    CoreBRCryptoAmount getFee();

    CoreBRCryptoFeeBasis getFeeBasis();

    Optional<CoreBRCryptoHash> getHash();

    int getDirection();

    int getState();

    boolean isIdentical(CoreBRCryptoTransfer other);

    BRCryptoTransfer asBRCryptoTransfer();
}
