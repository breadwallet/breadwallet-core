/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.util.List;

public interface CoreBRCryptoWallet {

    static CoreBRCryptoWallet createOwned(BRCryptoWallet wallet) {
        return new OwnedBRCryptoWallet(wallet);
    }

    BRCryptoAmount getBalance();

    List<CoreBRCryptoTransfer> getTransfers();

    boolean containsTransfer(CoreBRCryptoTransfer transfer);

    BRCryptoCurrency getCurrency();

    CoreBRCryptoUnit getUnitForFee();

    CoreBRCryptoUnit getUnit();

    int getState();

    void setState(int state);

    CoreBRCryptoFeeBasis getDefaultFeeBasis();

    void setDefaultFeeBasis(CoreBRCryptoFeeBasis feeBasis);

    BRCryptoAddress getSourceAddress(int addressScheme);

    BRCryptoAddress getTargetAddress(int addressScheme);

    CoreBRCryptoTransfer createTransfer(BRCryptoAddress target, BRCryptoAmount amount, CoreBRCryptoFeeBasis estimatedFeeBasis);

    Optional<CoreBRCryptoTransfer> createTransferForWalletSweep(BRCryptoWalletSweeper sweeper, CoreBRCryptoFeeBasis estimatedFeeBasis);

    void estimateFeeBasis(Pointer cookie, BRCryptoAddress target, BRCryptoAmount amount, CoreBRCryptoNetworkFee fee);

    void estimateFeeBasisForWalletSweep(Pointer cookie, BRCryptoWalletSweeper sweeper, CoreBRCryptoNetworkFee fee);

    BRCryptoWallet asBRCryptoWallet();
}
