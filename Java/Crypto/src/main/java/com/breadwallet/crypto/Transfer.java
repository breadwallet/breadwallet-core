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

public interface Transfer {

    Optional<Wallet> getWallet();

    Optional<Address> getSource();

    Optional<Address> getTarget();

    Amount getAmount();

    Amount getAmountDirected();

    Amount getFee();

    TransferFeeBasis getFeeBasis();

    TransferState getState();

    TransferDirection getDirection();

    Optional<TransferHash> getHash();

    default Optional<TransferConfirmation> getConfirmation() {
        TransferState state = getState();
        if (state.type == TransferState.Type.INCLUDED) {
            return Optional.of(state.includedConfirmation);
        }
        return Optional.absent();
    }

    default Optional<Long> getConfirmationsAt(long blockHeight) {
        Optional<TransferConfirmation> optionalConfirmation = getConfirmation();
        if (optionalConfirmation.isPresent()) {
            TransferConfirmation confirmation = optionalConfirmation.get();
            return (blockHeight >= confirmation.blockNumber) ? Optional.of(1 + blockHeight - confirmation.blockNumber) : Optional.absent();
        }
        return Optional.absent();
    }

    default Optional<Long> getConfirmations() {
        return getWallet().transform(wallet -> wallet.getWalletManager().getNetwork().getHeight());
    }
}
