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

import com.breadwallet.crypto.jni.bitcoin.BRTransaction;
import com.breadwallet.crypto.jni.CryptoLibrary;
import com.breadwallet.crypto.jni.bitcoin.BRWallet;
import com.breadwallet.crypto.jni.bitcoin.CoreBRWalletManager;
import com.google.common.base.Optional;

/* package */
class TransferBtcImpl extends Transfer {

    private final Wallet owner;

    private final BRWallet coreWallet;
    private final BRTransaction coreTransfer;

    private final Unit unit;
    private final TransferFeeBasis feeBasis;

    private TransferState state;

    /* package */
    TransferBtcImpl(Wallet owner, BRWallet coreWallet, BRTransaction coreTransfer, Unit unit) {
        this.owner = owner;
        this.coreWallet = coreWallet;
        this.coreTransfer = coreTransfer;
        this.unit = unit;
        this.state = TransferState.createCreated();

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
    boolean matches(BRTransaction transferImpl) {
        return coreTransfer.equals(transferImpl);
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
