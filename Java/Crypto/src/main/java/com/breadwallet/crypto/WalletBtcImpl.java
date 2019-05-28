package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.bitcoin.BRTransaction;
import com.breadwallet.crypto.jni.bitcoin.BRWallet;
import com.google.common.base.Optional;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import javax.annotation.Nullable;

import static com.google.common.base.Preconditions.checkState;

/* package */
final class WalletBtcImpl extends Wallet {

    private final List<Transfer> transfers;

    private final WalletManager owner;

    private final BRWallet coreWallet;

    private final Unit feeUnit;
    private final Unit defaultUnit;
    private final String name;

    private WalletState state;
    private TransferFeeBasis defaultFeeBasis;

    /* package */
    WalletBtcImpl(WalletManager owner, BRWallet wallet, Unit feeUnit, Unit defaultUnit) {
        this.owner = owner;
        this.coreWallet = wallet;
        this.feeUnit = feeUnit;
        this.defaultUnit = defaultUnit;
        this.name = defaultUnit.getCurrency().getCode();
        this.state = WalletState.CREATED;
        this.transfers = Collections.synchronizedList(new ArrayList<>());
        this.defaultFeeBasis = TransferFeeBasis.createBtc(coreWallet.getFeePerKb());
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
        return Amount.createAsBtc(coreWallet.getBalance(), defaultUnit);
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
    /* package */
    void setDefaultFeeBasis(TransferFeeBasis feeBasis) {
        defaultFeeBasis = feeBasis;
    }

    @Override
    public Address getTarget() {
        return Address.createAsBtc(coreWallet.legacyAddress());
    }

    @Override
    public Address getSource() {
        return Address.createAsBtc(coreWallet.legacyAddress());
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

        long feePerKbSaved = coreWallet.getFeePerKb();
        long feePerKb = feeBasis.getBtcFeePerKb();

        coreWallet.setFeePerKb(feePerKb);
        long fee = coreWallet.getFeeForTxAmount(amount.asBtc());
        coreWallet.setFeePerKb(feePerKbSaved);

        return Amount.createAsBtc(fee, feeUnit);
    }

    @Override
    boolean matches(BRWallet walletImpl) {
        return coreWallet.matches(walletImpl);
    }

    @Override
    /* package */
    Optional<Transfer> getOrCreateTransferByImpl(BRTransaction transferImpl, boolean createAllowed) {
        Optional<Transfer> optTransfer = getTransferByImpl(transferImpl);
        if (optTransfer.isPresent()) {
            return optTransfer;
        } else if (createAllowed) {
            return Optional.of(createTransferByImpl(transferImpl));
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
    Transfer createTransferByImpl(BRTransaction transferImpl) {
        Transfer transfer = new TransferBtcImpl(this, coreWallet, transferImpl, defaultUnit);
        transfers.add(transfer);
        return transfer;
    }

    private
    Optional<Transfer> getTransferByImpl(BRTransaction transferImpl) {
        for (Transfer transfer: transfers) {
            if (transfer.matches(transferImpl)) {
                return Optional.of(transfer);
            }
        }
        return Optional.absent();
    }

}
