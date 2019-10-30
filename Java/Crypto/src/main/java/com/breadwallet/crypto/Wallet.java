/*
 * Wallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;

import java.util.List;

public interface Wallet {

    /**
     * Create a TransferFeeBasis using a pricePerCostFactor and costFactor.
     *
     * Note: This is 'private' until the parameters are described.  Meant for testing for now.
     *
     */
    Optional<? extends TransferFeeBasis> createTransferFeeBasis(Amount pricePerCostFactor, double costFactor);

    Optional<? extends Transfer> createTransfer(Address target, Amount amount, TransferFeeBasis estimatedFeeBasis);

    void estimateFee(Address target, Amount amount, NetworkFee fee, CompletionHandler<TransferFeeBasis, FeeEstimationError> completion);

    WalletManager getWalletManager();

    Unit getUnit();

    Unit getUnitForFee();

    Amount getBalance();

    List<? extends Transfer> getTransfers();

    Optional<? extends Transfer> getTransferByHash(TransferHash hash);

    Address getTarget();

    Address getTargetForScheme(AddressScheme scheme);

    Address getSource();

    Currency getCurrency();

    String getName();

    WalletState getState();
}
