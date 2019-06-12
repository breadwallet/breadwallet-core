/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.google.common.primitives.UnsignedLong;

public interface CoreBRCryptoWallet {

    static CoreBRCryptoWallet create(BRCryptoWallet wallet) {
        return new OwnedBRCryptoWallet(wallet);
    }

    CoreBRCryptoAmount getBalance();

    UnsignedLong getTransferCount();

    CoreBRCryptoTransfer getTransfer(UnsignedLong index);

    int getState();

    void setState(int state);

    CoreBRCryptoFeeBasis getDefaultFeeBasis();

    void setDefaultFeeBasis(CoreBRCryptoFeeBasis feeBasis);

    CoreBRCryptoAddress getSourceAddress();

    CoreBRCryptoAddress getTargetAddress();

    CoreBRCryptoTransfer createTransfer(CoreBRCryptoAddress target, CoreBRCryptoAmount amount, CoreBRCryptoFeeBasis feeBasis);

    CoreBRCryptoAmount estimateFee(CoreBRCryptoAmount amount, CoreBRCryptoFeeBasis feeBasis, CoreBRCryptoUnit unit);

    BRCryptoWallet asBRCryptoWallet();
}
