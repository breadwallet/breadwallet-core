/*
 * Transfer
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import java.util.List;

public interface Transfer {

    Wallet getWallet();

    Optional<? extends Address> getSource();

    Optional<? extends Address> getTarget();

    Amount getAmount();

    Amount getAmountDirected();

    Amount getFee();

    TransferFeeBasis getFeeBasis();

    TransferDirection getDirection();

    Optional<? extends TransferHash> getHash();

    default Optional<TransferConfirmation> getConfirmation() {
        return getState().getIncludedConfirmation();
    }

    default Optional<UnsignedLong> getConfirmationsAt(UnsignedLong blockHeight) {
        Optional<TransferConfirmation> optionalConfirmation = getConfirmation();
        if (optionalConfirmation.isPresent()) {
            TransferConfirmation confirmation = optionalConfirmation.get();
            UnsignedLong blockNumber = confirmation.getBlockNumber();
            return blockHeight.compareTo(blockNumber) >= 0 ? Optional.of(UnsignedLong.ONE.plus(blockHeight).minus(blockNumber)) : Optional.absent();
        }
        return Optional.absent();
    }

    default Optional<UnsignedLong> getConfirmations() {
        return getConfirmationsAt(getWallet().getWalletManager().getNetwork().getHeight());
    }

    TransferState getState();
}
