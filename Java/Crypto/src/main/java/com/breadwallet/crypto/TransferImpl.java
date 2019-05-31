package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.bitcoin.BRTransaction;

import java.util.concurrent.atomic.AtomicReference;

/* package */
abstract class TransferImpl implements Transfer {

    protected final Wallet owner;
    protected final Unit defaultUnit;
    protected final AtomicReference<TransferState> state;

    protected TransferImpl(Wallet owner, Unit defaultUnit) {
        this.owner = owner;
        this.defaultUnit = defaultUnit;
        this.state = new AtomicReference<>(TransferState.CREATED());
    }

    @Override
    public Wallet getWallet() {
        return owner;
    }

    @Override
    public Amount getAmountDirected() {
        switch (getDirection()) {
            case RECOVERED:
                return Amount.create(0L, defaultUnit).get();
            case SENT:
                return getAmount().negate();
            case RECEIVED:
                return getAmount();
            default:
                throw new IllegalStateException("Invalid transfer direction");
        }
    }

    @Override
    public TransferState getState() {
        return state.get();
    }

    /* package */
    TransferState setState(TransferState newState) {
        return state.getAndSet(newState);
    }
}
