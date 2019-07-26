/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

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

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/* package */
final class Wallet implements com.breadwallet.crypto.Wallet {

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

        // TODO(fix): Figure out how we want to handle which thread/executor runs this
        Optional<TransferFeeBasis> optFeeBasis = core.estimateFeeBasis(coreAddress, coreAmount, coreFee).transform(TransferFeeBasis::create);
        if (optFeeBasis.isPresent()) {
            completion.handleData(optFeeBasis.get());
        } else {
            completion.handleError(new FeeEstimationServiceFailureError());
        }
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
}
