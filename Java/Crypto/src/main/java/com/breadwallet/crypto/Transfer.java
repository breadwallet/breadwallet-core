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

public abstract class Transfer {

    /* package */
    abstract Object getPointer();

    /* package */
    abstract TransferState setState(TransferState state);

    public abstract Optional<Address> getSource();

    public abstract Optional<Address> getTarget();

    public abstract Amount getAmount();

    public abstract Amount getAmountDirected();

    public abstract Amount getFee();

    public abstract TransferFeeBasis getFeeBasis();

    public abstract TransferState getState();

    public abstract TransferDirection getDirection();

    public abstract Optional<TransferHash> getHash();

    public abstract Optional<Long> getConfirmations();

    public Optional<TransferConfirmation> getConfirmation() {
        return getState().getIncludedConfirmation();
    }

    public Optional<Long> getConfirmationsAt(long blockHeight) {
        Optional<TransferConfirmation> optionalConfirmation = getConfirmation();
        if (optionalConfirmation.isPresent()) {
            TransferConfirmation confirmation = optionalConfirmation.get();
            return (blockHeight >= confirmation.blockNumber) ? Optional.of(1 + blockHeight - confirmation.blockNumber) : Optional.absent();
        }
        return Optional.absent();
    }
}
