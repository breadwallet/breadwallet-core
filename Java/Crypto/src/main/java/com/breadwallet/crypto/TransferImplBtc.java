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
import com.breadwallet.crypto.jni.bitcoin.BRTxInput;
import com.breadwallet.crypto.jni.bitcoin.BRTxOutput;
import com.breadwallet.crypto.jni.bitcoin.BRWallet;
import com.breadwallet.crypto.jni.bitcoin.CoreBRTransaction;
import com.breadwallet.crypto.jni.support.UInt256;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

/* package */
final class TransferImplBtc extends TransferImpl {

    private final BRWallet coreWallet;
    private final CoreBRTransaction coreTransfer;

    /* package */
    TransferImplBtc(Wallet owner, BRWallet coreWallet, CoreBRTransaction coreTransfer, Unit defaultUnit) {
        super(owner, defaultUnit);
        this.coreWallet = coreWallet;
        this.coreTransfer = coreTransfer;
    }

    @Override
    public byte[] serialize() {
        return coreTransfer.serialize();
    }

    @Override
    public Optional<Address> getSource() {
        boolean sent = UnsignedLong.MAX_VALUE.longValue() != coreWallet.getFeeForTx(coreTransfer);

        for (BRTxInput input: coreTransfer.getInputs()) {
            String addressStr = input.getAddressAsString();
            if (sent == coreWallet.containsAddress(addressStr)) {
                return Address.createAsBtc(addressStr);
            }
        }
        return Optional.absent();
    }

    @Override
    public Optional<Address> getTarget() {
        boolean sent = UnsignedLong.MAX_VALUE.longValue() != coreWallet.getFeeForTx(coreTransfer);

        for (BRTxOutput output: coreTransfer.getOutputs()) {
            String addressStr = output.getAddressAsString();
            if (!sent == coreWallet.containsAddress(addressStr)) {
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
        return TransferFeeBasis.createBtc(BRWallet.DEFAULT_FEE_PER_KB);
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
        UInt256 txHash = coreTransfer.getTxHash();
        for (byte value: txHash.u8) {
            if (value != 0) {
                return Optional.of(TransferHash.createBtc(txHash));
            }
        }
        return Optional.absent();
    }

    /* package */
    CoreBRTransaction getCoreBRTransaction() {
        return coreTransfer;
    }

    /* package */
    boolean matches(BRTransaction transferImpl) {
        return coreTransfer.equals(transferImpl);
    }
}
