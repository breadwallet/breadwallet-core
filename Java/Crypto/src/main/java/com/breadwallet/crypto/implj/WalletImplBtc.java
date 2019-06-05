/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferFeeBasis;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.libcrypto.bitcoin.BRTransaction;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWallet;
import com.breadwallet.crypto.libcrypto.bitcoin.CoreBRTransaction;
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
        // TODO(fix): The swift equivalent will result in this being added to 'transfers' and an event being created; do we want this?
        String addr = target.toString();
        long value = AmountImpl.from(amount).integerRawAmount();
        Unit unit = amount.getUnit();
        return CoreBRTransaction.create(coreWallet, value, addr).transform((t) -> new TransferImplBtc(this, coreWallet, t, unit));
    }

    @Override
    public Amount estimateFee(Amount amount, TransferFeeBasis feeBasis) {
        checkState(amount.hasCurrency(defaultUnit.getCurrency()));

        long feePerKbSaved = coreWallet.getFeePerKb();
        long feePerKb = feeBasis.getBtcFeePerKb();

        coreWallet.setFeePerKb(feePerKb);
        long fee = coreWallet.getFeeForTxAmount(AmountImpl.from(amount).integerRawAmount());
        coreWallet.setFeePerKb(feePerKbSaved);

        return AmountImpl.createAsBtc(fee, feeUnit);
    }

    @Override
    public Amount getBalance() {
        return AmountImpl.createAsBtc(coreWallet.getBalance(), defaultUnit);
    }

    @Override
    public Address getTarget() {
        return AddressImpl.createAsBtc(coreWallet.legacyAddress());
    }

    @Override
    public Address getSource() {
        return AddressImpl.createAsBtc(coreWallet.legacyAddress());
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
