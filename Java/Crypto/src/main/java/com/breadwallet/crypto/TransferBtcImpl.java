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
import com.breadwallet.crypto.jni.CryptoLibrary.BRCryptoBoolean;
import com.breadwallet.crypto.jni.bitcoin.BRTxInput;
import com.breadwallet.crypto.jni.bitcoin.BRTxOutput;
import com.breadwallet.crypto.jni.bitcoin.BRWallet;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

/* package */
final class TransferBtcImpl extends Transfer {

    private final Wallet owner;

    private final BRWallet coreWallet;
    private final BRTransaction coreTransfer;

    private final Unit defaultUnit;

    private TransferState state;

    /* package */
    TransferBtcImpl(Wallet owner, BRWallet coreWallet, BRTransaction coreTransfer, Unit defaultUnit) {
        this.owner = owner;
        this.coreWallet = coreWallet;
        this.coreTransfer = coreTransfer;
        this.defaultUnit = defaultUnit;
        this.state = TransferState.createCreated();
    }

    @Override
    public Wallet getWallet() {
        return owner;
    }

    @Override
    public Optional<Address> getSource() {
        boolean sent = UnsignedLong.MAX_VALUE.longValue() != coreWallet.getFeeForTx(coreTransfer);

        BRTxInput inputs[] = (BRTxInput[]) coreTransfer.inputs.toArray(coreTransfer.inCount.intValue());
        int inputsContain = sent ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE;

        for (BRTxInput input: inputs) {
            String addressStr = input.getAddressAsString();
            if (inputsContain == coreWallet.containsAddress(addressStr)) {
                return Address.createAsBtc(addressStr);
            }
        }
        return Optional.absent();
    }

    @Override
    public Optional<Address> getTarget() {
        boolean sent = UnsignedLong.MAX_VALUE.longValue() != coreWallet.getFeeForTx(coreTransfer);

        BRTxOutput outputs[] = (BRTxOutput[]) coreTransfer.outputs.toArray(coreTransfer.outCount.intValue());
        int inputsContain = sent ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE;

        for (BRTxOutput output: outputs) {
            String addressStr = output.getAddressAsString();
            if (inputsContain == coreWallet.containsAddress(addressStr)) {
                return Address.createAsBtc(addressStr);
            }
        }
        return Optional.absent();
    }

    @Override
    public Amount getAmount() {
        long fee = coreWallet.getFeeForTx(coreTransfer);
        if (fee == UnsignedLong.MAX_VALUE.longValue()) {
            fee = 0;
        }

        long recv = coreWallet.getAmountReceivedFromTx(coreTransfer);
        long send  = coreWallet.getAmountSentByTx(coreTransfer);

        switch (getDirection()) {
            case RECOVERED:
                return Amount.createAsBtc(send, defaultUnit);
            case SENT:
                return Amount.createAsBtc(send - recv - fee, defaultUnit);
            case RECEIVED:
                return Amount.createAsBtc(recv, defaultUnit);
            default:
                throw new IllegalStateException("Invalid transfer direction");
        }
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
    public Amount getFee() {
        long fee = coreWallet.getFeeForTx(coreTransfer);
        if (fee == UnsignedLong.MAX_VALUE.longValue()) {
            fee = 0;
        }
        return Amount.createAsBtc(fee, defaultUnit);
    }

    @Override
    public TransferFeeBasis getFeeBasis() {
        // TODO: There is a comment in the Swift about this; is this OK?
        return TransferFeeBasis.createBtc(CryptoLibrary.DEFAULT_FEE_PER_KB);
    }

    @Override
    public TransferState getState() {
        return state;
    }

    @Override
    public TransferDirection getDirection() {
        long fee = coreWallet.getFeeForTx(coreTransfer);
        if (fee == UnsignedLong.MAX_VALUE.longValue()) {
            fee = 0;
        }

        long recv = coreWallet.getAmountReceivedFromTx(coreTransfer);
        long send  = coreWallet.getAmountSentByTx(coreTransfer);

        if (send > 0 && (recv + fee) == send) {
            return TransferDirection.RECOVERED;

        } else if (send > 0) {
            return TransferDirection.SENT;

        } else {
            return TransferDirection.RECEIVED;
        }
    }

    @Override
    public Optional<TransferHash> getHash() {
        for (byte value: coreTransfer.txHash.u8) {
            if (value != 0) {
                return Optional.of(TransferHash.createBtc(coreTransfer.txHash));
            }
        }
        return Optional.absent();
    }

    @Override
    public Optional<Long> getConfirmations() {
        // TODO: Think we should not be doing this; instead pass in network
        return getConfirmationsAt(owner.getWalletManager().getNetwork().getHeight());
    }

    @Override
    /* package */
    boolean matches(BRTransaction transferImpl) {
        return coreTransfer.equals(transferImpl);
    }

    @Override
    /* package */
    TransferState setState(TransferState newState) {
        // TODO: Do we want to synchronize here or are we ok with only being done by SystemImpl on a single thread?
        TransferState oldState = state;
        state = newState;
        return oldState;
    }
}
