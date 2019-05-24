/*
 * Transfer
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.BRTransaction;
import com.breadwallet.crypto.jni.CryptoLibrary;
import com.google.common.base.Optional;
import com.sun.jna.Pointer;

// TODO: Where do we free 'core'?

/* package */
class TransferBtcImpl extends Transfer {

    private final BRTransaction core;

    private final Wallet owner;
    private final Unit unit;
    private final TransferFeeBasis feeBasis;

    private TransferState state;

    /* package */
    TransferBtcImpl(Wallet owner, Pointer ptr, Unit unit) {
        this.owner = owner;
        this.unit = unit;
        this.state = TransferState.createCreated();

        this.core = new BRTransaction(ptr);

        // TODO: There is a comment in the Swift about this; is this OK?
        this.feeBasis = TransferFeeBasis.createBtc(CryptoLibrary.DEFAULT_FEE_PER_KB);
    }

    @Override
    public Wallet getWallet() {
        return owner;
    }

    @Override
    public Optional<Address> getSource() {
        // TODO: Implement this
        return null;
    }

    @Override
    public Optional<Address> getTarget() {
        // TODO: Implement this
        return null;
    }

    @Override
    public Amount getAmount() {
        // TODO: Implement this
        return null;
    }

    @Override
    public Amount getAmountDirected() {
        // TODO: Implement this
        return null;
    }

    @Override
    public Amount getFee() {
        // TODO: Implement this
        return null;
    }

    @Override
    public TransferFeeBasis getFeeBasis() {
        return feeBasis;
    }

    @Override
    public TransferState getState() {
        return state;
    }

    @Override
    public TransferDirection getDirection() {
        // TODO: Implement this
        return null;
    }

    @Override
    public Optional<TransferHash> getHash() {
        // TODO: Implement this
        return null;
    }

    @Override
    public Optional<Long> getConfirmations() {
        // TODO: Implement this
        return null;
    }

    @Override
    /* package */
    Pointer getPointer() {
        return core.getPointer();
    }

    @Override
    /* package */
    TransferState setState(TransferState newState) {
        // TODO: Do we want to synchronize here?
        TransferState oldState = state;
        state = newState;
        return oldState;
    }
}
