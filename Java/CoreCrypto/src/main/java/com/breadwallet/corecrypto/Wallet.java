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
import com.breadwallet.corenative.crypto.CoreBRCryptoTransfer;
import com.breadwallet.corenative.crypto.CoreBRCryptoUnit;
import com.breadwallet.corenative.crypto.CoreBRCryptoWallet;
import com.breadwallet.crypto.WalletState;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

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

    private final Unit feeUnit;
    private final Unit defaultUnit;
    private final Currency defaultUnitCurrency;

    private Wallet(CoreBRCryptoWallet core, WalletManager walletManager) {
        this.core = core;
        this.walletManager = walletManager;

        this.feeUnit = Unit.create(core.getFeeUnit());
        this.defaultUnit = Unit.create(core.getUnit());
        this.defaultUnitCurrency = Currency.create(core.getCurrency());
    }

    @Override
    public Optional<Transfer> createTransfer(com.breadwallet.crypto.Address target,
                                             com.breadwallet.crypto.Amount amount) {
        return createTransfer(target, amount, getDefaultFeeBasis());
    }

    @Override
    public Optional<Transfer> createTransfer(com.breadwallet.crypto.Address target,
                                             com.breadwallet.crypto.Amount amount,
                                             com.breadwallet.crypto.TransferFeeBasis feeBasis) {
        CoreBRCryptoAddress coreAddress = Address.from(target).getCoreBRCryptoAddress();
        CoreBRCryptoFeeBasis coreFeeBasis = TransferFeeBasis.from(feeBasis).getCoreBRFeeBasis();
        Amount cryptoAmount = Amount.from(amount);
        Unit cryptoUnit = cryptoAmount.getUnit();
        CoreBRCryptoTransfer transfer = core.createTransfer(coreAddress, cryptoAmount.getCoreBRCryptoAmount(), coreFeeBasis);
        return Optional.of(Transfer.create(transfer, this, cryptoUnit));
    }

    @Override
    public Amount estimateFee(com.breadwallet.crypto.Amount amount) {
        return estimateFee(amount, getDefaultFeeBasis());
    }

    @Override
    public Amount estimateFee(com.breadwallet.crypto.Amount amount, com.breadwallet.crypto.TransferFeeBasis feeBasis) {
        CoreBRCryptoAmount coreAmount = Amount.from(amount).getCoreBRCryptoAmount();
        CoreBRCryptoFeeBasis coreFeeBasis = TransferFeeBasis.from(feeBasis).getCoreBRFeeBasis();
        CoreBRCryptoUnit coreUnit = feeUnit.getCoreBRCryptoUnit();
        return Amount.create(core.estimateFee(coreAmount, coreFeeBasis, coreUnit), feeUnit);
    }

    @Override
    public WalletManager getWalletManager() {
        return walletManager;
    }

    @Override
    public List<Transfer> getTransfers() {
        List<Transfer> transfers = new ArrayList<>();

        for (CoreBRCryptoTransfer transfer: core.getTransfers()) {
            transfers.add(Transfer.create(transfer, this, defaultUnit));
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
    public Unit getBaseUnit() {
        return defaultUnit;
    }

    @Override
    public Amount getBalance() {
        return Amount.create(core.getBalance(), defaultUnit);
    }

    @Override
    public WalletState getState() {
        return Utilities.walletStateFromCrypto(core.getState());
    }

    @Override
    public TransferFeeBasis getDefaultFeeBasis() {
        return TransferFeeBasis.create(core.getDefaultFeeBasis());
    }

    @Override
    public Address getTarget() {
        return Address.create(core.getTargetAddress());
    }

    @Override
    public Address getSource() {
        return Address.create(core.getSourceAddress());
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
                Optional.of(Transfer.create(transfer, this, defaultUnit)) :
                Optional.absent();
    }

    /* package */
    Optional<Transfer> getTransferOrCreate(CoreBRCryptoTransfer transfer) {
        Optional<Transfer> optional = getTransfer(transfer);
        if (optional.isPresent()) {
            return optional;

        } else {
            return Optional.of(Transfer.create(transfer, this, defaultUnit));
        }
    }

    /* package */
    CoreBRCryptoWallet getCoreBRCryptoWallet() {
        return core;
    }
}
