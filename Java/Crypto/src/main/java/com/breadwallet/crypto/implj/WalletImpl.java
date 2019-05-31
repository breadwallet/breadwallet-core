package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.Transfer;
import com.breadwallet.crypto.TransferFeeBasis;
import com.breadwallet.crypto.TransferHash;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.WalletManager;
import com.breadwallet.crypto.WalletState;
import com.google.common.base.Optional;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/* package */
abstract class WalletImpl<T extends TransferImpl> implements Wallet {

    protected final WalletManager owner;
    protected final Unit feeUnit;
    protected final Unit defaultUnit;
    protected final AtomicReference<WalletState> state;

    protected final Lock transfersReadLock;
    protected final Lock transfersWriteLock;
    protected final List<T> transfers;

    protected TransferFeeBasis defaultFeeBasis;

    protected WalletImpl(WalletManager owner, Unit feeUnit, Unit defaultUnit, TransferFeeBasis defaultFeeBasis) {
        this.owner = owner;
        this.feeUnit = feeUnit;
        this.defaultUnit = defaultUnit;
        this.defaultFeeBasis = defaultFeeBasis;
        this.state = new AtomicReference<>(WalletState.CREATED);

        ReadWriteLock transfersRwLock = new ReentrantReadWriteLock();
        this.transfersReadLock = transfersRwLock.readLock();
        this.transfersWriteLock = transfersRwLock.writeLock();
        this.transfers = new ArrayList<>();
    }

    @Override
    public Amount estimateFee(Amount amount) {
        return estimateFee(amount, defaultFeeBasis);
    }

    @Override
    public WalletManager getWalletManager() {
        return owner;
    }

    @Override
    public List<Transfer> getTransfers() {
        transfersReadLock.lock();
        try {
            return new ArrayList<>(transfers);
        } finally {
            transfersReadLock.unlock();
        }
    }

    @Override
    public Optional<Transfer> getTransferByHash(TransferHash hash) {
        transfersReadLock.lock();
        try {
            for (Transfer transfer : transfers) {
                TransferHash transferHash = transfer.getHash().orNull();
                if (hash.equals(transferHash)) {
                    return Optional.of(transfer);
                }
            }
            return Optional.absent();
        } finally {
            transfersReadLock.unlock();
        }
    }

    @Override
    public Unit getBaseUnit() {
        return defaultUnit;
    }

    @Override
    public WalletState getState() {
        return state.get();
    }

    /* package */
    WalletState setState(WalletState newState) {
        return state.getAndSet(newState);
    }

    @Override
    public TransferFeeBasis getDefaultFeeBasis() {
        return defaultFeeBasis;
    }

    /* package */
    void setDefaultFeeBasis(TransferFeeBasis feeBasis) {
        defaultFeeBasis = feeBasis;
    }
}
