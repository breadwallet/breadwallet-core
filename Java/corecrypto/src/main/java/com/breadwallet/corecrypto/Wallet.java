/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoAddress;
import com.breadwallet.corenative.crypto.BRCryptoAmount;
import com.breadwallet.corenative.crypto.BRCryptoFeeBasis;
import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.breadwallet.corenative.crypto.BRCryptoPaymentProtocolRequest;
import com.breadwallet.corenative.crypto.BRCryptoTransfer;
import com.breadwallet.corenative.crypto.BRCryptoWallet;
import com.breadwallet.corenative.crypto.BRCryptoWalletManager;
import com.breadwallet.corenative.crypto.BRCryptoWalletSweeper;
import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.WalletState;
import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.errors.LimitEstimationError;
import com.breadwallet.crypto.errors.LimitEstimationInsufficientFundsError;
import com.breadwallet.crypto.errors.LimitEstimationServiceFailureError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

import static com.google.common.base.Preconditions.checkState;

/* package */
final class Wallet implements com.breadwallet.crypto.Wallet {

    /* package */
    static Wallet takeAndCreate(BRCryptoWallet core, WalletManager walletManager, SystemCallbackCoordinator callbackCoordinator) {
        return Wallet.create(core.take(), walletManager, callbackCoordinator);
    }

    /* package */
    static Wallet create(BRCryptoWallet core, WalletManager walletManager, SystemCallbackCoordinator callbackCoordinator) {
        Wallet wallet = new Wallet(core, walletManager, callbackCoordinator);
        ReferenceCleaner.register(wallet, core::give);
        return wallet;
    }

    /* package */
    static Wallet from(com.breadwallet.crypto.Wallet wallet) {
        if (wallet == null) {
            return null;
        }

        if (wallet instanceof Wallet) {
            return (Wallet) wallet;
        }

        throw new IllegalArgumentException("Unsupported wallet instance");
    }

    private final BRCryptoWallet core;
    private final WalletManager walletManager;
    private final SystemCallbackCoordinator callbackCoordinator;

    private final Supplier<Unit> unitSupplier;
    private final Supplier<Unit> unitForFeeSupplier;
    private final Supplier<Currency> defaultUnitCurrencySupplier;

    private Wallet(BRCryptoWallet core, WalletManager walletManager, SystemCallbackCoordinator callbackCoordinator) {
        this.core = core;
        this.walletManager = walletManager;
        this.callbackCoordinator = callbackCoordinator;

        this.unitSupplier = Suppliers.memoize(() -> Unit.create(core.getUnit()));
        this.unitForFeeSupplier = Suppliers.memoize(() -> Unit.create(core.getUnitForFee()));
        this.defaultUnitCurrencySupplier = Suppliers.memoize(() -> Currency.create(core.getCurrency()));
    }

    @Override
    public Optional<TransferFeeBasis> createTransferFeeBasis(com.breadwallet.crypto.Amount pricePerCostFactor, double costFactor) {
        BRCryptoAmount corePricePerCostFactor = Amount.from(pricePerCostFactor).getCoreBRCryptoAmount();
        return core.createTransferFeeBasis(corePricePerCostFactor, costFactor).transform(TransferFeeBasis::create);
    }

    @Override
    public Optional<Transfer> createTransfer(com.breadwallet.crypto.Address target,
                                             com.breadwallet.crypto.Amount amount,
                                             com.breadwallet.crypto.TransferFeeBasis estimatedFeeBasis) {
        BRCryptoAddress coreAddress = Address.from(target).getCoreBRCryptoAddress();
        BRCryptoFeeBasis coreFeeBasis = TransferFeeBasis.from(estimatedFeeBasis).getCoreBRFeeBasis();
        BRCryptoAmount coreAmount = Amount.from(amount).getCoreBRCryptoAmount();
        return core.createTransfer(coreAddress, coreAmount, coreFeeBasis).transform(t -> Transfer.create(t, this));
    }

    /* package */
    Optional<Transfer> createTransfer(WalletSweeper sweeper,
                                      com.breadwallet.crypto.TransferFeeBasis estimatedFeeBasis) {
        BRCryptoWalletSweeper coreSweeper = sweeper.getCoreBRWalletSweeper();
        BRCryptoFeeBasis coreFeeBasis = TransferFeeBasis.from(estimatedFeeBasis).getCoreBRFeeBasis();
        return core.createTransferForWalletSweep(coreSweeper, coreFeeBasis).transform(t -> Transfer.create(t, this));
    }

    /* package */
    Optional<Transfer> createTransfer(PaymentProtocolRequest request,
                                      com.breadwallet.crypto.TransferFeeBasis estimatedFeeBasis) {
        BRCryptoPaymentProtocolRequest coreRequest = request.getBRCryptoPaymentProtocolRequest();
        BRCryptoFeeBasis coreFeeBasis = TransferFeeBasis.from(estimatedFeeBasis).getCoreBRFeeBasis();
        return core.createTransferForPaymentProtocolRequest(coreRequest, coreFeeBasis).transform(t -> Transfer.create(t, this));
    }

    @Override
    public void estimateFee(com.breadwallet.crypto.Address target, com.breadwallet.crypto.Amount amount,
                            com.breadwallet.crypto.NetworkFee fee, CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> handler) {
        BRCryptoWalletManager coreManager = getWalletManager().getCoreBRCryptoWalletManager();
        BRCryptoAddress coreAddress = Address.from(target).getCoreBRCryptoAddress();
        BRCryptoAmount coreAmount = Amount.from(amount).getCoreBRCryptoAmount();
        BRCryptoNetworkFee coreFee = NetworkFee.from(fee).getCoreBRCryptoNetworkFee();
        coreManager.estimateFeeBasis(core, callbackCoordinator.registerFeeBasisEstimateHandler(handler), coreAddress, coreAmount, coreFee);
    }

    /* package */
    void estimateFee(WalletSweeper sweeper,
                     com.breadwallet.crypto.NetworkFee fee, CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> handler) {
        BRCryptoWalletManager coreManager = getWalletManager().getCoreBRCryptoWalletManager();
        BRCryptoWalletSweeper coreSweeper = sweeper.getCoreBRWalletSweeper();
        BRCryptoNetworkFee coreFee = NetworkFee.from(fee).getCoreBRCryptoNetworkFee();
        coreManager.estimateFeeBasisForWalletSweep(core, callbackCoordinator.registerFeeBasisEstimateHandler(handler), coreSweeper, coreFee);
    }

    /* package */
    void estimateFee(PaymentProtocolRequest request,
                     com.breadwallet.crypto.NetworkFee fee, CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> handler) {
        BRCryptoWalletManager coreManager = getWalletManager().getCoreBRCryptoWalletManager();
        BRCryptoPaymentProtocolRequest coreRequest = request.getBRCryptoPaymentProtocolRequest();
        BRCryptoNetworkFee coreFee = NetworkFee.from(fee).getCoreBRCryptoNetworkFee();
        coreManager.estimateFeeBasisForPaymentProtocolRequest(core, callbackCoordinator.registerFeeBasisEstimateHandler(handler), coreRequest, coreFee);
    }

    @Override
    public void estimateLimitMaximum(com.breadwallet.crypto.Address target, com.breadwallet.crypto.NetworkFee fee,
                                     CompletionHandler<com.breadwallet.crypto.Amount, LimitEstimationError> handler) {
        estimateLimit(true, target, fee, handler);
    }

    @Override
    public void estimateLimitMinimum(com.breadwallet.crypto.Address target, com.breadwallet.crypto.NetworkFee fee,
                                     CompletionHandler<com.breadwallet.crypto.Amount, LimitEstimationError> handler) {
        estimateLimit(false, target, fee, handler);
    }

    private void estimateLimit(boolean asMaximum,
                               com.breadwallet.crypto.Address target, com.breadwallet.crypto.NetworkFee fee,
                               CompletionHandler<com.breadwallet.crypto.Amount, LimitEstimationError> handler) {
        BRCryptoWalletManager coreManager = getWalletManager().getCoreBRCryptoWalletManager();

        NetworkFee cryptoFee = NetworkFee.from(fee);
        BRCryptoNetworkFee coreFee = cryptoFee.getCoreBRCryptoNetworkFee();
        BRCryptoAddress coreAddress = Address.from(target).getCoreBRCryptoAddress();

        // This `amount` is in the `unit` of `wallet`
        BRCryptoWalletManager.EstimateLimitResult result = coreManager.estimateLimit(core, asMaximum, coreAddress, coreFee);
        if (result.amount == null) {
            // This is extraneous as `cryptoWalletEstimateLimit()` always returns an amount
            callbackCoordinator.completeLimitEstimateWithError(handler, new LimitEstimationInsufficientFundsError());
            return;
        }

        Amount amount = Amount.create(result.amount);
        boolean needFeeEstimate = result.needFeeEstimate;
        boolean isZeroIfInsuffientFunds = result.isZeroIfInsuffientFunds;

        // If we don't need an estimate, then we invoke `completion` and skip out immediately.  But
        // include a check on a zero amount - which indicates insufficient funds.
        if (!needFeeEstimate) {
            if (isZeroIfInsuffientFunds && amount.isZero()) {
                callbackCoordinator.completeLimitEstimateWithError(handler, new LimitEstimationInsufficientFundsError());
            } else {
                callbackCoordinator.completeLimitEstimateWithSuccess(handler, amount);
            }
            return;
        }

        // We need an estimate of the fees.

        // The currency for the fee
        Currency currencyForFee = cryptoFee.getPricePerCostFactor().getCurrency();

        Wallet walletForFee = null;
        for (Wallet wallet: walletManager.getWallets()) {
            if (currencyForFee.equals(wallet.getCurrency())) {
                walletForFee = wallet;
                break;
            }
        }
        if (null == walletForFee) {
            callbackCoordinator.completeLimitEstimateWithError(handler, new LimitEstimationServiceFailureError());
            return;
        }

        // Skip out immediately if we've no balance.
        if (walletForFee.getBalance().isZero()) {
            callbackCoordinator.completeLimitEstimateWithError(handler, new LimitEstimationInsufficientFundsError());
            return;
        }

        //
        // If the `walletForFee` differs from `wallet` then we just need to estimate the fee
        // once.  Get the fee estimate and just ensure that walletForFee has sufficient balance
        // to pay the fee.
        //
        if (!this.equals(walletForFee)) {
            // This `amount` will not unusually be zero.
            // TODO: Does ETH fee estimation work if the ERC20 amount is zero?
            final Wallet walletForFeeInner = walletForFee;
            estimateFee(target, amount, fee, new CompletionHandler<com.breadwallet.crypto.TransferFeeBasis,
                    FeeEstimationError>() {
                @Override
                public void handleData(com.breadwallet.crypto.TransferFeeBasis feeBasis) {
                    if (walletForFeeInner.getBalance().compareTo(feeBasis.getFee()) >= 0) {
                        handler.handleData(amount);
                    } else {
                        handler.handleError(new LimitEstimationInsufficientFundsError());
                    }
                }

                @Override
                public void handleError(FeeEstimationError error) {
                    handler.handleError(LimitEstimationError.from(error));
                }
            });
            return;
        }

        // The `fee` is in the same unit as the `wallet`

        //
        // If we are estimating the minimum, then get the fee and ensure that the wallet's
        // balance is enough to cover the (minimum) amount plus the fee
        //
        if (!asMaximum) {
            estimateFee(target, amount, fee, new CompletionHandler<com.breadwallet.crypto.TransferFeeBasis,
                    FeeEstimationError>() {
                @Override
                public void handleData(com.breadwallet.crypto.TransferFeeBasis feeBasis) {
                    Optional<Amount> transactionAmount = amount.add(feeBasis.getFee());
                    checkState(transactionAmount.isPresent());

                    if (getBalance().compareTo(transactionAmount.get()) >= 0) {
                        handler.handleData(amount);
                    } else {
                        handler.handleError(new LimitEstimationInsufficientFundsError());
                    }
                }

                @Override
                public void handleError(FeeEstimationError error) {
                    handler.handleError(LimitEstimationError.from(error));
                }
            });
            return;
        }

        // This function will be recursively defined
        CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> estimationHandler =
                new CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError>() {
            // If the `walletForFee` and `wallet` are identical, then we need to iteratively estimate
            // the fee and adjust the amount until the fee stabilizes.
            com.breadwallet.crypto.Amount transferFee = Amount.create(0, getUnit());

            // We'll limit the number of iterations
            int estimationCompleterRecurseLimit = 3;
            int estimationCompleterRecurseCount = 0;

            @Override
            public void handleData(com.breadwallet.crypto.TransferFeeBasis feeBasis) {
                estimationCompleterRecurseCount += 1;

                // The estimated transfer fee
                com.breadwallet.crypto.Amount newTransferFee = feeBasis.getFee();

                // The estimated transfer amount, updated with the transferFee
                Optional<Amount> newTransferAmount = amount.sub(newTransferFee);
                checkState(newTransferAmount.isPresent());

                // If the two transfer fees match, then we have converged
                if (transferFee.equals(newTransferFee)) {
                    Optional<Amount> transactionAmount = newTransferAmount.get().add(newTransferFee);
                    checkState(transactionAmount.isPresent());

                    if (getBalance().compareTo(transactionAmount.get()) >= 0) {
                        handler.handleData(newTransferAmount.get());
                    } else {
                        handler.handleError(new LimitEstimationInsufficientFundsError());
                    }

                } else if (estimationCompleterRecurseCount < estimationCompleterRecurseLimit) {
                    // but is they haven't converged try again with the new amount
                    transferFee = newTransferFee;
                    estimateFee(target, newTransferAmount.get(), fee, this);

                } else {
                    // We've tried too many times w/o convergence; abort
                    handler.handleError(new LimitEstimationServiceFailureError());
                }
            }

            @Override
            public void handleError(FeeEstimationError error) {
                handler.handleError(LimitEstimationError.from(error));
            }
        };

        estimateFee(target, amount, fee, estimationHandler);
    }

    @Override
    public WalletManager getWalletManager() {
        return walletManager;
    }

    @Override
    public List<Transfer> getTransfers() {
        List<Transfer> transfers = new ArrayList<>();

        for (BRCryptoTransfer transfer: core.getTransfers()) {
            transfers.add(Transfer.create(transfer, this));
        }

        return transfers;
    }

    @Override
    public Optional<Transfer> getTransferByHash(com.breadwallet.crypto.TransferHash hash) {
        List<Transfer> transfers = getTransfers();

        for (Transfer transfer : transfers) {
            Optional<TransferHash> optional = transfer.getHash();
            if (optional.isPresent() && optional.get().equals(hash)) {
                return Optional.of(transfer);
            }
        }
        return Optional.absent();
    }

    @Override
    public Unit getUnit() {
        return unitSupplier.get();
    }

    @Override
    public Unit getUnitForFee() {
        return unitForFeeSupplier.get();
    }

    @Override
    public Amount getBalance() {
        return Amount.create(core.getBalance());
    }

    @Override
    public WalletState getState() {
        return Utilities.walletStateFromCrypto(core.getState());
    }

    @Override
    public Address getTarget() {
        return getTargetForScheme(walletManager.getAddressScheme());
    }

    @Override
    public Address getTargetForScheme(AddressScheme scheme) {
        return Address.create(core.getTargetAddress(Utilities.addressSchemeToCrypto(scheme)));
    }

    @Override
    public boolean containsAddress(com.breadwallet.crypto.Address address) {
        return core.containsAddress(Address.from(address).getCoreBRCryptoAddress());
    }

    @Override
    public Currency getCurrency() {
        return defaultUnitCurrencySupplier.get();
    }

    @Override
    public String getName() {
        return getCurrency().getCode();
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof Wallet)) {
            return false;
        }

        Wallet that = (Wallet) object;
        return core.equals(that.core);
    }

    @Override
    public String toString() {
        return "Wallet{" +
                "currency=" + getName() +
                '}';
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }

    /* package */
    Optional<Transfer> getTransfer(BRCryptoTransfer transfer) {
        return core.containsTransfer(transfer) ?
                Optional.of(Transfer.takeAndCreate(transfer, this)) :
                Optional.absent();
    }

    /* package */
    Transfer createTransfer(BRCryptoTransfer transfer) {
        return Transfer.takeAndCreate(transfer, this);
    }

    /* package */
    BRCryptoWallet getCoreBRCryptoWallet() {
        return core;
    }
}
