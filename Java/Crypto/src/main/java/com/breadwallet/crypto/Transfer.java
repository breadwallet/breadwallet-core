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

import java.util.List;

public interface Transfer {

    Wallet getWallet();

    Optional<Address> getSource();

    Optional<Address> getTarget();

    List<Address> getSources();

    List<Address> getTargets();

    List<Address> getInputs();

    List<Address> getOutputs();

    Amount getAmount();

    Amount getAmountDirected();

    Amount getFee();

    TransferFeeBasis getFeeBasis();

    TransferDirection getDirection();

    Optional<TransferHash> getHash();

    default Optional<TransferConfirmation> getConfirmation() {
        return getState().getIncludedConfirmation();
    }

    default Optional<Long> getConfirmationsAt(long blockHeight) {
        Optional<TransferConfirmation> optionalConfirmation = getConfirmation();
        if (optionalConfirmation.isPresent()) {
            TransferConfirmation confirmation = optionalConfirmation.get();
            long blockNumber = confirmation.getBlockNumber();
            return (blockHeight >= blockNumber) ? Optional.of(1 + blockHeight - blockNumber) : Optional.absent();
        }
        return Optional.absent();
    }

    default Optional<Long> getConfirmations() {
        return getConfirmationsAt(getWallet().getWalletManager().getNetwork().getHeight());
    }

    TransferState getState();
}
