package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.CryptoLibrary.BRWallet;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;

import java.util.ArrayList;
import java.util.List;

public class WalletBtcImpl extends Wallet {

    private final Object owner;
    private final BRWallet core;
    private final Unit unit;
    private final WalletState state;
    private final String name;

    private TransferFeeBasis defaultFeeBasis;

    /* package */
    WalletBtcImpl(Object owner, Pointer ptr, Unit unit) {
        this.owner = owner;
        this.core = new BRWallet(ptr);
        this.unit = unit;
        this.name = unit.getCurrency().getCode();
        this.state = WalletState.CREATED;
        this.defaultFeeBasis = TransferFeeBasis.createBtc(CryptoLibrary.INSTANCE.BRWalletFeePerKb(core));
    }

    @Override
    Pointer getPointer() {
        return core.getPointer();
    }

    @Override
    public Unit getBaseUnit() {
        return unit;
    }

    @Override
    public Amount getBalance() {
        // TODO: This diverges from Swift, is this okay?
        return Amount.create(CryptoLibrary.INSTANCE.BRWalletBalance(core), unit);
    }

    @Override
    public List<Transfer> getTransfers() {
        // TODO: Implement me!
        return new ArrayList<>();
    }

    @Override
    public Optional<Transfer> getTransferByHash(TransferHash hash) {
        // TODO: Implement me!
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
        // TODO: Implement me!
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
    public Amount estimateFee(Amount amount, TransferFeeBasis feeBasis) {
        // TODO: Implement me!
        return null;
    }
}
