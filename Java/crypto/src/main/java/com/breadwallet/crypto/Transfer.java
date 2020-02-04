/*
 * Transfer
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.events.transfer.TransferChangedEvent;
import com.breadwallet.crypto.events.walletmanager.WalletManagerBlockUpdatedEvent;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import java.util.Set;

public interface Transfer {

    Wallet getWallet();

    Optional<? extends Address> getSource();

    Optional<? extends Address> getTarget();

    Amount getAmount();

    Amount getAmountDirected();

    Amount getFee();

    Optional<? extends TransferFeeBasis> getEstimatedFeeBasis();

    Optional<? extends TransferFeeBasis> getConfirmedFeeBasis();

    TransferDirection getDirection();

    Optional<? extends TransferHash> getHash();

    Unit getUnit();

    Unit getUnitForFee();

    default Optional<TransferConfirmation> getConfirmation() {
        return getState().getIncludedConfirmation();
    }

    /**
     * Get the number of confirmations of transfer at a provided <code>blockHeight</code>.
     *
     * If the transfer has not been confirmed or if the <code>blockHeight</code> is less than the confirmation height,
     * <code>absent</code> is returned.
     *
     * The minimum returned value is 1; if <code>blockHeight</code> is the same as the confirmation block, then the
     * transfer has been confirmed once.
     */
    default Optional<UnsignedLong> getConfirmationsAt(UnsignedLong blockHeight) {
        Optional<TransferConfirmation> optionalConfirmation = getConfirmation();
        if (optionalConfirmation.isPresent()) {
            TransferConfirmation confirmation = optionalConfirmation.get();
            UnsignedLong blockNumber = confirmation.getBlockNumber();
            return blockHeight.compareTo(blockNumber) >= 0 ? Optional.of(UnsignedLong.ONE.plus(blockHeight).minus(blockNumber)) : Optional.absent();
        }
        return Optional.absent();
    }

    /**
     * Get the number of confirmations of transfer at the current network height.
     *
     * Since this value is calculated based on the associated network's height, it is recommended that a developer
     * refreshes any cached result in response to {@link WalletManagerBlockUpdatedEvent} events on the owning
     * WalletManager, in addition to further {@link TransferChangedEvent} events on this Transfer.
     *
     * If the transfer has not been confirmed or if the network's height is less than the confirmation height,
     * <code>absent</code> is returned.
     *
     * The minimum returned value is 1; if the height is the same as the confirmation block, then the transfer has
     * been confirmed once.
     */
    default Optional<UnsignedLong> getConfirmations() {
        return getConfirmationsAt(getWallet().getWalletManager().getNetwork().getHeight());
    }

    TransferState getState();

    Set<? extends TransferAttribute> getAttributes ();
}
