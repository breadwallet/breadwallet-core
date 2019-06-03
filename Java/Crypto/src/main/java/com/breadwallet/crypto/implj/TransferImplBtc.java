/*
 * Transfer
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.Address;
import com.breadwallet.crypto.Amount;
import com.breadwallet.crypto.TransferDirection;
import com.breadwallet.crypto.TransferFeeBasis;
import com.breadwallet.crypto.TransferHash;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.crypto.libcrypto.bitcoin.BRTransaction;
import com.breadwallet.crypto.libcrypto.bitcoin.BRTxInput;
import com.breadwallet.crypto.libcrypto.bitcoin.BRTxOutput;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWallet;
import com.breadwallet.crypto.libcrypto.bitcoin.CoreBRTransaction;
import com.breadwallet.crypto.libcrypto.support.UInt256;
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
    public Optional<Address> getSource() {
        boolean sent = UnsignedLong.MAX_VALUE.longValue() != coreWallet.getFeeForTx(coreTransfer);

        for (BRTxInput input: coreTransfer.getInputs()) {
            String addressStr = input.getAddressAsString();
            if (sent == coreWallet.containsAddress(addressStr)) {
                return AddressImpl.createAsBtc(addressStr).transform(a -> a);
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
                return AddressImpl.createAsBtc(addressStr).transform(a -> a);
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
                return AmountImpl.createAsBtc(send, defaultUnit);
            case SENT:
                return AmountImpl.createAsBtc(send - recv - fee, defaultUnit);
            case RECEIVED:
                return AmountImpl.createAsBtc(recv, defaultUnit);
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
        return AmountImpl.createAsBtc(fee, defaultUnit);
    }

    @Override
    public TransferFeeBasis getFeeBasis() {
        // TODO(discuss): There is a comment in the Swift about this; is this OK?
        return TransferFeeBasis.createBtc(BRWallet.DEFAULT_FEE_PER_KB);
    }

    @Override
    public TransferDirection getDirection() {
        // TODO(fix): these calculations as a whole (amount, direction, etc.) need to be revisited
        long fee = coreWallet.getFeeForTx(coreTransfer);
        if (fee == UnsignedLong.MAX_VALUE.longValue()) {
            fee = 0;
        }

        long recv = coreWallet.getAmountReceivedFromTx(coreTransfer);
        long send  = coreWallet.getAmountSentByTx(coreTransfer);

        if (send > 0 && (recv + fee) == send) {
            return TransferDirection.RECOVERED;

        } else if (send > (recv + fee)) {
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
                return Optional.of(TransferHashImpl.createBtc(txHash));
            }
        }
        return Optional.absent();
    }

    /* package */
    byte[] serialize() {
        return coreTransfer.serialize();
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
