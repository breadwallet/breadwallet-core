/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.util.Log;

import com.breadwallet.corenative.crypto.BRCryptoBoolean;
import com.breadwallet.corenative.crypto.BRCryptoWalletEstimateFeeBasisResult;
import com.breadwallet.corenative.crypto.CoreBRCryptoAddress;
import com.breadwallet.corenative.crypto.CoreBRCryptoAmount;
import com.breadwallet.corenative.crypto.CoreBRCryptoFeeBasis;
import com.breadwallet.corenative.crypto.CoreBRCryptoNetworkFee;
import com.breadwallet.corenative.crypto.CoreBRCryptoTransfer;
import com.breadwallet.corenative.crypto.CoreBRCryptoWallet;
import com.breadwallet.crypto.AddressScheme;
import com.breadwallet.crypto.WalletState;
import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.errors.FeeEstimationServiceFailureError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

/* package */
final class Wallet implements com.breadwallet.crypto.Wallet {

    private static final String TAG = System.class.getName();

    private static final AtomicInteger ESTIMATE_FEE_CALLBACK_IDS = new AtomicInteger(0);

    private static final Map<Pointer, CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError>> ESTIMATE_FEE_CALLBACKS = new ConcurrentHashMap<>();

    /* package */
    static Wallet create(CoreBRCryptoWallet wallet, WalletManager walletManager) {
        return new Wallet(wallet, walletManager);
    }

    private final CoreBRCryptoWallet core;
    private final WalletManager walletManager;

    private final Unit unitForFee;
    private final Unit unit;
    private final Currency defaultUnitCurrency;

    private Wallet(CoreBRCryptoWallet core, WalletManager walletManager) {
        this.core = core;
        this.walletManager = walletManager;

        this.unit = Unit.create(core.getUnit());
        this.unitForFee = Unit.create(core.getUnitForFee());
        this.defaultUnitCurrency = Currency.create(core.getCurrency());
    }

    @Override
    public Optional<Transfer> createTransfer(com.breadwallet.crypto.Address target,
                                             com.breadwallet.crypto.Amount amount,
                                             com.breadwallet.crypto.TransferFeeBasis estimatedFeeBasis) {
        CoreBRCryptoAddress coreAddress = Address.from(target).getCoreBRCryptoAddress();
        CoreBRCryptoFeeBasis coreFeeBasis = TransferFeeBasis.from(estimatedFeeBasis).getCoreBRFeeBasis();
        CoreBRCryptoAmount coreAmount = Amount.from(amount).getCoreBRCryptoAmount();
        return Optional.of(Transfer.create(core.createTransfer(coreAddress, coreAmount, coreFeeBasis), this));
    }

    @Override
    public void estimateFee(com.breadwallet.crypto.Address target, com.breadwallet.crypto.Amount amount,
                            com.breadwallet.crypto.NetworkFee fee, CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> completion) {
        CoreBRCryptoAddress coreAddress = Address.from(target).getCoreBRCryptoAddress();
        CoreBRCryptoAmount coreAmount = Amount.from(amount).getCoreBRCryptoAmount();
        CoreBRCryptoNetworkFee coreFee = NetworkFee.from(fee).getCoreBRCryptoNetworkFee();
        core.estimateFeeBasis(coreAddress, coreAmount, coreFee, addEstimateFeeBasisCallback(completion), Wallet::estimateFeeBasisCallback);
    }

    @Override
    public WalletManager getWalletManager() {
        return walletManager;
    }

    @Override
    public List<Transfer> getTransfers() {
        List<Transfer> transfers = new ArrayList<>();

        for (CoreBRCryptoTransfer transfer: core.getTransfers()) {
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
        return unit;
    }

    @Override
    public Unit getUnitForFee() {
        return unitForFee;
    }

    @Override
    public Amount getBalance() {
        return Amount.create(core.getBalance(), unit);
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
        return defaultUnitCurrency;
    }

    @Override
    public String getName() {
        return defaultUnitCurrency.getCode();
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
    void setDefaultFeeBasis(com.breadwallet.crypto.TransferFeeBasis feeBasis) {
        core.setDefaultFeeBasis(TransferFeeBasis.from(feeBasis).getCoreBRFeeBasis());
    }

    /* package */
    void setState(WalletState newState) {
        core.setState(Utilities.walletStateToCrypto(newState));
    }

    /* package */
    Optional<Transfer> getTransfer(CoreBRCryptoTransfer transfer) {
        return core.containsTransfer(transfer) ?
                Optional.of(Transfer.create(transfer, this)) :
                Optional.absent();
    }

    /* package */
    Optional<Transfer> getTransferOrCreate(CoreBRCryptoTransfer transfer) {
        Optional<Transfer> optional = getTransfer(transfer);
        if (optional.isPresent()) {
            return optional;

        } else {
            return Optional.of(Transfer.create(transfer, this));
        }
    }

    /* package */
    CoreBRCryptoWallet getCoreBRCryptoWallet() {
        return core;
    }

    // static callbacks

    private static Pointer addEstimateFeeBasisCallback(CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> completion) {
        Pointer context = Pointer.createConstant(ESTIMATE_FEE_CALLBACK_IDS.incrementAndGet());
        ESTIMATE_FEE_CALLBACKS.put(context, completion);
        return context;
    }

    private static Optional<CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError>> removeEstimateFeeBasisCallback(Pointer context) {
        return Optional.fromNullable(ESTIMATE_FEE_CALLBACKS.remove(context));
    }

    private static void estimateFeeBasisCallback(Pointer pointer, BRCryptoWalletEstimateFeeBasisResult.ByValue result) {
        Log.d(TAG, "BRCryptoWalletEstimateFeeBasisCallback");

        Optional<CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError>> maybeCompletion = removeEstimateFeeBasisCallback(pointer);
        if (maybeCompletion.isPresent()) {
            Log.d(TAG, "BRCryptoWalletEstimateFeeBasisCallback: executing callback");

            if (result.success == BRCryptoBoolean.CRYPTO_TRUE) {
                maybeCompletion.get().handleData(TransferFeeBasis.create(CoreBRCryptoFeeBasis.createOwned(result.u.success.feeBasis)));
            } else {
                maybeCompletion.get().handleError(new FeeEstimationServiceFailureError());
            }
        } else {
            Log.e(TAG, "BRCryptoWalletEstimateFeeBasisCallback: missing callback");
        }
    }
}
