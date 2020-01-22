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
import com.breadwallet.crypto.errors.LimitEstimationError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;

import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

public interface Wallet {

    /**
     * Create a TransferFeeBasis using a pricePerCostFactor and costFactor.
     *
     * Note: This is 'private' until the parameters are described.  Meant for testing for now.
     *
     */
    Optional<? extends TransferFeeBasis> createTransferFeeBasis(Amount pricePerCostFactor, double costFactor);

    Optional<? extends Transfer> createTransfer(Address target, Amount amount, TransferFeeBasis estimatedFeeBasis, @Nullable Set<TransferAttribute> attributes);

    /**
     * Estimate the fee for a transfer with `amount` from `wallet`.  If provided use the `feeBasis`
     * otherwise use the wallet's `defaultFeeBasis`
     *
     * @param target the transfer's target address
     * @param amount the transfer amount MUST BE GREATER THAN 0
     * @param fee the network fee (aka priority)
     * @param completion handler function
     */
    void estimateFee(Address target, Amount amount, NetworkFee fee, CompletionHandler<TransferFeeBasis, FeeEstimationError> completion);

    /**
     * Estimate the maximum amount that can be transfered from Wallet.
     *
     * This value does not include the fee, however, a fee estimate has been performed and the maximum has been
     * adjusted to be (nearly) balance = amount + fee.  That is, the maximum amount is what you can safe transfer to
     * 'zero out' the wallet
     *
     * In cases where `balance < fee` then {@link com.breadwallet.crypto.errors.LimitEstimationInsufficientFundsError}
     * is returned.  This can occur for an ERC20 transfer where the ETH wallet's balance is not enough to pay the fee.
     * That is, the {@link com.breadwallet.crypto.errors.LimitEstimationInsufficientFundsError} check respects the
     * wallet from which fees are extracted.  Both BTC and ETH transfer might have an insufficient balance to pay a fee.
     *
     * This is an synchronous function that returns immediately but will call `completion` once the maximum has been
     * determined.
     *
     * The returned Amount is always in the wallet's currency.
     *
     * @param target the target address
     * @param fee the network fees
     * @param completion the handler for the results
    */
    void estimateLimitMaximum(Address target, NetworkFee fee, CompletionHandler<Amount, LimitEstimationError> completion);

    /**
     * Estimate the minimum amount that can be transfered from Wallet.
     *
     * This value does not include the fee, however, a fee estimate has been performed. Generally the minimum
     * amount in zero; however, some currencies have minimum values, below which miners will
     * reject.  In those cases the minimum amount is above zero.
     *
     * In cases where `balance < amount + fee` then {@link com.breadwallet.crypto.errors.LimitEstimationInsufficientFundsError}
     * is returned.  The {@link com.breadwallet.crypto.errors.LimitEstimationInsufficientFundsError} check respects the
     * wallet from which fees are extracted.
     *
     * This is an synchronous function that returns immediately but will call `completion` once
     * the maximum has been determined.
     *
     * The returned Amount is always in the wallet's currencyh.
     *
     * The returned Amount is always in the wallet's currency.
     * @param target the target address
     * @param fee the network fees
     * @param completion the handler for the results
     */
    void estimateLimitMinimum(Address target, NetworkFee fee, CompletionHandler<Amount, LimitEstimationError> completion);

    WalletManager getWalletManager();

    Unit getUnit();

    Unit getUnitForFee();

    Amount getBalance();

    Optional<? extends Amount> getBalanceMaximum();

    Optional<? extends Amount> getBalanceMinimum();

    List<? extends Transfer> getTransfers();

    Optional<? extends Transfer> getTransferByHash(TransferHash hash);

    Set<? extends TransferAttribute> getTransferAttributesFor (@Nullable Address address);

    default Set<? extends TransferAttribute> getTransferAttributes () {
        return getTransferAttributesFor(null);
    }

    Optional<TransferAttribute.Error> validateTransferAttribute(TransferAttribute attribute);

    Optional<TransferAttribute.Error>  validateTransferAttributes(Set<TransferAttribute> attributes);

    Address getTarget();

    Address getTargetForScheme(AddressScheme scheme);

    boolean containsAddress(Address address);

    Currency getCurrency();

    String getName();

    WalletState getState();
}
