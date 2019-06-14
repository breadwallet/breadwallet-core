/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import android.support.annotation.Nullable;

import com.breadwallet.corenative.crypto.CoreBRCryptoTransfer;
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
    static Wallet create(CoreBRCryptoWallet wallet, WalletManager walletManager, Unit networkBaseUnit, Unit networkDefaultUnit) {
        return new Wallet(wallet, walletManager, networkBaseUnit, networkDefaultUnit);
    }

    private final CoreBRCryptoWallet core;
    private final WalletManager walletManager;
    private final Unit feeUnit;
    private final Unit defaultUnit;
    private final Currency defaultUnitCurrency;

    private TransferFeeBasis defaultFeeBasis;

    private Wallet(CoreBRCryptoWallet core, WalletManager walletManager, Unit networkBaseUnit, Unit networkDefaultUnit) {
        this.core = core;
        this.walletManager = walletManager;
        this.feeUnit = networkBaseUnit;
        this.defaultUnit = networkDefaultUnit;
        this.defaultUnitCurrency = networkDefaultUnit.getCurrency();
        this.defaultFeeBasis = TransferFeeBasis.create(core.getDefaultFeeBasis());
    }

    @Override
    public Optional<Transfer> createTransfer(com.breadwallet.crypto.Address target,
                                             com.breadwallet.crypto.Amount amount) {
        return createTransfer(target, amount, defaultFeeBasis);
    }

    @Override
    public Optional<Transfer> createTransfer(com.breadwallet.crypto.Address target,
                                             com.breadwallet.crypto.Amount amount,
                                             com.breadwallet.crypto.TransferFeeBasis feeBasis) {
        Address targetImpl = Address.from(target);
        Amount amountImpl = Amount.from(amount);
        TransferFeeBasis feeBasisImpl = TransferFeeBasis.from(feeBasis);
        Unit unitImpl = amountImpl.getUnit();
        CoreBRCryptoTransfer transfer = core.createTransfer(targetImpl.getCoreBRCryptoAddress(),
                amountImpl.getCoreBRCryptoAmount(), feeBasisImpl.getCoreBRFeeBasis());
        return Optional.of(Transfer.create(transfer, this, unitImpl));
    }

    @Override
    public Amount estimateFee(com.breadwallet.crypto.Amount amount) {
        return estimateFee(amount, defaultFeeBasis);
    }

    @Override
    public Amount estimateFee(com.breadwallet.crypto.Amount amount, com.breadwallet.crypto.TransferFeeBasis feeBasis) {
        Amount amountImpl = Amount.from(amount);
        TransferFeeBasis feeBasisImpl = TransferFeeBasis.from(feeBasis);
        return Amount.create(core.estimateFee(amountImpl.getCoreBRCryptoAmount(), feeBasisImpl.getCoreBRFeeBasis(),
                defaultUnit.getCoreBRCryptoUnit()), feeUnit);
    }

    @Override
    public WalletManager getWalletManager() {
        return walletManager;
    }

    @Override
    public List<Transfer> getTransfers() {
        List<Transfer> transfers = new ArrayList<>();

        UnsignedLong count = core.getTransferCount();
        for (UnsignedLong i = UnsignedLong.ZERO; i.compareTo(count) < 0; i = i.plus(UnsignedLong.ONE)) {
            transfers.add(Transfer.create(core.getTransfer(i), this, defaultUnit));
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
        return defaultFeeBasis;
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
        defaultFeeBasis = TransferFeeBasis.from(feeBasis);
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
