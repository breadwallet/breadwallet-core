package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.bitcoin.BRTransaction;
import com.breadwallet.crypto.jni.bitcoin.BRWallet;
import com.breadwallet.crypto.jni.bitcoin.CoreBRTransaction;
import com.google.common.base.Optional;

import static com.google.common.base.Preconditions.checkState;

/* package */
final class WalletImplBtc extends WalletImpl<TransferImplBtc> {

    private final BRWallet coreWallet;

    /* package */
    WalletImplBtc(WalletManager owner, BRWallet wallet, Unit feeUnit, Unit defaultUnit) {
        super(owner, feeUnit, defaultUnit, TransferFeeBasis.createBtc(wallet.getFeePerKb()));
        this.coreWallet = wallet;
    }

    @Override
    public Optional<Transfer> createTransfer(Address target, Amount amount, TransferFeeBasis feeBasis) {
        // TODO: The swift equivalent will result in this being added to 'transfers' and an event being created; do we want this?
        String addr = target.toString();
        long value = amount.integerAmount();
        Unit unit = amount.getUnit();
        return CoreBRTransaction.create(coreWallet, value, addr).transform((t) -> new TransferImplBtc(this, coreWallet, t, unit));
    }

    @Override
    public Amount estimateFee(Amount amount, TransferFeeBasis feeBasis) {
        checkState(amount.hasCurrency(defaultUnit.getCurrency()));

        long feePerKbSaved = coreWallet.getFeePerKb();
        long feePerKb = feeBasis.getBtcFeePerKb();

        coreWallet.setFeePerKb(feePerKb);
        long fee = coreWallet.getFeeForTxAmount(amount.integerAmount());
        coreWallet.setFeePerKb(feePerKbSaved);

        return Amount.createAsBtc(fee, feeUnit);
    }

    @Override
    public Amount getBalance() {
        return Amount.createAsBtc(coreWallet.getBalance(), defaultUnit);
    }

    @Override
    public Address getTarget() {
        return Address.createAsBtc(coreWallet.legacyAddress());
    }

    @Override
    public Address getSource() {
        return Address.createAsBtc(coreWallet.legacyAddress());
    }

    /* package */
    boolean matches(BRWallet walletImpl) {
        return coreWallet.matches(walletImpl);
    }

    /* package */
    Optional<TransferImplBtc> getOrCreateTransferByImpl(BRTransaction transferImpl, boolean createAllowed) {
        transfersWriteLock.lock();
        try {
            Optional<TransferImplBtc> optTransfer = getTransferByImplUnderLock(transferImpl);
            if (optTransfer.isPresent()) {
                return optTransfer;
            } else if (createAllowed) {
                return Optional.of(addTransferByImplUnderLock(transferImpl));
            } else {
                return Optional.absent();
            }
        } finally {
            transfersWriteLock.unlock();
        }
    }

    private TransferImplBtc addTransferByImplUnderLock(BRTransaction transferImpl) {
        TransferImplBtc transfer = new TransferImplBtc(this, coreWallet, transferImpl, defaultUnit);
        transfers.add(transfer);
        return transfer;
    }

    private Optional<TransferImplBtc> getTransferByImplUnderLock(BRTransaction transferImpl) {
        for (TransferImplBtc transfer: transfers) {
            if (transfer.matches(transferImpl)) {
                return Optional.of(transfer);
            }
        }
        return Optional.absent();
    }
}
