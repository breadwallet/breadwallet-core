package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRWallet;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import javax.annotation.Nullable;

import static com.google.common.base.Preconditions.checkArgument;
import static com.google.common.base.Preconditions.checkState;

// TODO: Where do we free 'core'?

/* package */
class WalletBtcImpl extends Wallet {

    private final List<Transfer> transfers;

    private final BRWallet core;

    private final WalletManager owner;
    private final Unit feeUnit;
    private final Unit defaultUnit;
    private final String name;

    private WalletState state;
    private TransferFeeBasis defaultFeeBasis;

    /* package */
    WalletBtcImpl(WalletManager owner, Pointer ptr, Unit feeUnit, Unit defaultUnit) {
        this.owner = owner;
        this.feeUnit = feeUnit;
        this.defaultUnit = defaultUnit;
        this.name = defaultUnit.getCurrency().getCode();
        this.state = WalletState.CREATED;

        this.transfers = Collections.synchronizedList(new ArrayList<>());
        this.core = new BRWallet(ptr);

        this.defaultFeeBasis = TransferFeeBasis.createBtc(CryptoLibrary.INSTANCE.BRWalletFeePerKb(core));
    }

    @Override
    public WalletManager getWalletManager() {
        return owner;
    }

    @Override
    public Unit getBaseUnit() {
        return defaultUnit;
    }

    @Override
    public Amount getBalance() {
        // TODO: This diverges from Swift, is this okay?
        return Amount.create(CryptoLibrary.INSTANCE.BRWalletBalance(core), defaultUnit);
    }

    @Override
    public List<Transfer> getTransfers() {
        return new ArrayList<>(transfers);
    }

    @Override
    public Optional<Transfer> getTransferByHash(TransferHash hash) {
        for (Transfer transfer: transfers) {
            TransferHash transferHash = transfer.getHash().orNull();
            if (hash.equals(transferHash )) {
                return Optional.of(transfer);
            }
        }
        return Optional.absent();
    }

    @Override
    public WalletState getState() {
        return state;
    }

    @Override
    public TransferFeeBasis getDefaultFeeBasis() {
        return defaultFeeBasis;
    }

    @Override
    public void setDefaultFeeBasis(TransferFeeBasis feeBasis) {
        defaultFeeBasis = feeBasis;
    }

    @Override
    public Address getTarget() {
        return Address.createAsBtc(CryptoLibrary.INSTANCE.BRWalletLegacyAddress(core));
    }

    @Override
    public Address getSource() {
        return Address.createAsBtc(CryptoLibrary.INSTANCE.BRWalletLegacyAddress(core));
    }

    @Override
    public Optional<Transfer> createTransfer(Address target, Amount amount, TransferFeeBasis feeBasis) {
        // TODO: Implement me!
        return Optional.absent();
    }

    @Override
    public Amount estimateFee(Amount amount, @Nullable TransferFeeBasis feeBasis) {
        checkState(amount.hasCurrency(defaultUnit.getCurrency()));
        feeBasis = feeBasis == null ? defaultFeeBasis : feeBasis;

        long feePerKbSaved = CryptoLibrary.INSTANCE.BRWalletFeePerKb(core);

        Optional<Long> optFeePerKb = feeBasis.getBtcFeePerKb();
        checkArgument(optFeePerKb.isPresent());
        long feePerKb = optFeePerKb.get();

        CryptoLibrary.INSTANCE.BRWalletSetFeePerKb(core, feePerKb);
        long fee = CryptoLibrary.INSTANCE.BRWalletFeeForTxAmount(core, amount.asBtc());
        CryptoLibrary.INSTANCE.BRWalletSetFeePerKb(core, feePerKbSaved);

        // TODO: This diverges from Swift, is this okay?
        return Amount.create(fee, feeUnit);
    }

    @Override
    Pointer getPointer() {
        return core.getPointer();
    }

    @Override
    /* package */
    Optional<Transfer> getOrCreateTransferByPtr(Pointer transferPtr, boolean createAllowed) {
        Optional<Transfer> optTransfer = getTransferByPtr(transferPtr);
        if (optTransfer.isPresent()) {
            return optTransfer;
        } else if (createAllowed) {
            return Optional.of(createTransferByPtr(transferPtr));
        } else {
            return Optional.absent();
        }
    }

    @Override
    /* package */
    WalletState setState(WalletState newState) {
        WalletState oldState = state;
        state = newState;
        return oldState;
    }

    private
    Transfer createTransferByPtr(Pointer transferPtr) {
        Transfer transfer = new TransferBtcImpl(this, transferPtr, defaultUnit);
        transfers.add(transfer);
        return transfer;
    }

    private
    Optional<Transfer> getTransferByPtr(Pointer transferPtr) {
        for (Transfer transfer: transfers) {
            if (transfer.getPointer().equals(transferPtr)) {
                return Optional.of(transfer);
            }
        }
        return Optional.absent();
    }

}
