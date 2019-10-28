/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.util.Log;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoAddress;
import com.breadwallet.corenative.crypto.BRCryptoAmount;
import com.breadwallet.corenative.crypto.BRCryptoFeeBasis;
import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.breadwallet.corenative.crypto.BRCryptoTransfer;
import com.breadwallet.corenative.crypto.BRCryptoWallet;
import com.breadwallet.corenative.crypto.BRCryptoWalletSweeper;
import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.WalletState;
import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/* package */
final class Wallet implements com.breadwallet.crypto.Wallet {

    private static final String TAG = Wallet.class.getName();

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

    @Override
    public void estimateFee(com.breadwallet.crypto.Address target, com.breadwallet.crypto.Amount amount,
                            com.breadwallet.crypto.NetworkFee fee, CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> handler) {
        BRCryptoAddress coreAddress = Address.from(target).getCoreBRCryptoAddress();
        BRCryptoAmount coreAmount = Amount.from(amount).getCoreBRCryptoAmount();
        BRCryptoNetworkFee coreFee = NetworkFee.from(fee).getCoreBRCryptoNetworkFee();
        core.estimateFeeBasis(callbackCoordinator.registerFeeBasisEstimateHandler(handler), coreAddress, coreAmount, coreFee);
    }

    /* package */
    void estimateFee(WalletSweeper sweeper,
                     com.breadwallet.crypto.NetworkFee fee, CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> handler) {
        BRCryptoWalletSweeper coreSweeper = sweeper.getCoreBRWalletSweeper();
        BRCryptoNetworkFee coreFee = NetworkFee.from(fee).getCoreBRCryptoNetworkFee();
        core.estimateFeeBasisForWalletSweep(callbackCoordinator.registerFeeBasisEstimateHandler(handler), coreSweeper, coreFee);
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
    public Address getSource() {
        return Address.create(core.getSourceAddress(Utilities.addressSchemeToCrypto(walletManager.getAddressScheme())));
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
