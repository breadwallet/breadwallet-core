/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferFeeBasis;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.corenative.bitcoin.BRTransaction;
import com.breadwallet.corenative.bitcoin.BRWallet;
import com.breadwallet.corenative.bitcoin.CoreBRTransaction;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import static com.google.common.base.Preconditions.checkState;

/* package */
final class WalletBtc extends Wallet<TransferBtc> {

    private final BRWallet coreWallet;

    /* package */
    WalletBtc(WalletManager owner, BRWallet wallet, Unit feeUnit, Unit defaultUnit) {
        super(owner, feeUnit, defaultUnit, TransferFeeBasis.createBtc(wallet.getFeePerKb()));
        this.coreWallet = wallet;
    }

    @Override
    public Optional<Transfer> createTransfer(com.breadwallet.crypto.Address target, com.breadwallet.crypto.Amount amount, TransferFeeBasis feeBasis) {
        // TODO(fix): The swift equivalent will result in this being added to 'transfers' and an event being created; do we want this?
        String addr = target.toString();
        UnsignedLong value = Amount.from(amount).integerRawAmount();
        Unit unit = amount.getUnit();
        return CoreBRTransaction.create(coreWallet, value, addr).transform((t) -> new TransferBtc(this, coreWallet, t, unit));
    }

    @Override
    public com.breadwallet.crypto.Amount estimateFee(com.breadwallet.crypto.Amount amount, TransferFeeBasis feeBasis) {
        checkState(amount.hasCurrency(defaultUnit.getCurrency()));

        UnsignedLong feePerKbSaved = coreWallet.getFeePerKb();
        UnsignedLong feePerKb = feeBasis.getBtcFeePerKb();

        coreWallet.setFeePerKb(feePerKb);
        UnsignedLong fee = coreWallet.getFeeForTxAmount(Amount.from(amount).integerRawAmount());
        coreWallet.setFeePerKb(feePerKbSaved);

        return Amount.createAsBtc(fee, feeUnit);
    }

    @Override
    public com.breadwallet.crypto.Amount getBalance() {
        return Amount.createAsBtc(coreWallet.getBalance(), defaultUnit);
    }

    @Override
    public com.breadwallet.crypto.Address getTarget() {
        return Address.createAsBtc(coreWallet.legacyAddress());
    }

    @Override
    public com.breadwallet.crypto.Address getSource() {
        return Address.createAsBtc(coreWallet.legacyAddress());
    }

    /* package */
    boolean matches(BRWallet walletImpl) {
        return coreWallet.matches(walletImpl);
    }

    /* package */
    Optional<TransferBtc> getOrCreateTransferByImpl(BRTransaction transferImpl, boolean createAllowed) {
        transfersWriteLock.lock();
        try {
            Optional<TransferBtc> optTransfer = getTransferByImplUnderLock(transferImpl);
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

    private TransferBtc addTransferByImplUnderLock(BRTransaction transferImpl) {
        TransferBtc transfer = new TransferBtc(this, coreWallet, transferImpl, defaultUnit);
        transfers.add(transfer);
        return transfer;
    }

    private Optional<TransferBtc> getTransferByImplUnderLock(BRTransaction transferImpl) {
        for (TransferBtc transfer: transfers) {
            if (transfer.matches(transferImpl)) {
                return Optional.of(transfer);
            }
        }
        return Optional.absent();
    }
}
