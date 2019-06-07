/*
 * Transfer
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.TransferDirection;
import com.breadwallet.crypto.TransferFeeBasis;
import com.breadwallet.crypto.Unit;
import com.breadwallet.crypto.Wallet;
import com.breadwallet.corenative.bitcoin.BRTransaction;
import com.breadwallet.corenative.bitcoin.BRTxInput;
import com.breadwallet.corenative.bitcoin.BRTxOutput;
import com.breadwallet.corenative.bitcoin.BRWallet;
import com.breadwallet.corenative.bitcoin.CoreBRTransaction;
import com.breadwallet.corenative.support.UInt256;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

import java.util.ArrayList;
import java.util.List;

import static com.google.common.base.Preconditions.checkState;

/* package */
final class TransferBtc extends Transfer {

    private final BRWallet coreWallet;
    private final CoreBRTransaction coreTransfer;

    /* package */
    TransferBtc(Wallet owner, BRWallet coreWallet, CoreBRTransaction coreTransfer, Unit defaultUnit) {
        super(owner, defaultUnit);
        this.coreWallet = coreWallet;
        this.coreTransfer = coreTransfer;
    }

    @Override
    public Optional<com.breadwallet.crypto.Address> getSource() {
        boolean sent = !UnsignedLong.MAX_VALUE.equals(coreWallet.getFeeForTx(coreTransfer));

        for (BRTxInput input: coreTransfer.getInputs()) {
            String addressStr = input.getAddressAsString();
            if (sent == coreWallet.containsAddress(addressStr)) {
                return Address.createAsBtc(addressStr).transform(a -> a);
            }
        }
        return Optional.absent();
    }

    @Override
    public Optional<com.breadwallet.crypto.Address> getTarget() {
        boolean sent = !UnsignedLong.MAX_VALUE.equals(coreWallet.getFeeForTx(coreTransfer));

        for (BRTxOutput output: coreTransfer.getOutputs()) {
            String addressStr = output.getAddressAsString();
            if (!sent == coreWallet.containsAddress(addressStr)) {
                return Address.createAsBtc(addressStr).transform(a -> a);
            }
        }
        return Optional.absent();
    }

    @Override
    public List<com.breadwallet.crypto.Address> getSources() {
        boolean sent = !UnsignedLong.MAX_VALUE.equals(coreWallet.getFeeForTx(coreTransfer));

        List<com.breadwallet.crypto.Address> addresses = new ArrayList<>();
        for (BRTxInput input: coreTransfer.getInputs()) {
            String addressStr = input.getAddressAsString();
            if (sent == coreWallet.containsAddress(addressStr)) {
                Optional<Address> optional = Address.createAsBtc(addressStr);
                if (optional.isPresent()) addresses.add(optional.get());
            }
        }
        return addresses;
    }

    @Override
    public List<com.breadwallet.crypto.Address> getTargets() {
        boolean sent = !UnsignedLong.MAX_VALUE.equals(coreWallet.getFeeForTx(coreTransfer));

        List<com.breadwallet.crypto.Address> addresses = new ArrayList<>();
        for (BRTxOutput output: coreTransfer.getOutputs()) {
            String addressStr = output.getAddressAsString();
            if (!sent == coreWallet.containsAddress(addressStr)) {
                Optional<Address> optional = Address.createAsBtc(addressStr);
                if (optional.isPresent()) addresses.add(optional.get());
            }
        }
        return addresses;
    }

    @Override
    public List<com.breadwallet.crypto.Address> getInputs() {
        List<com.breadwallet.crypto.Address> addresses = new ArrayList<>();
        for (BRTxInput input: coreTransfer.getInputs()) {
            String addressStr = input.getAddressAsString();
            Optional<Address> optional = Address.createAsBtc(addressStr);
            if (optional.isPresent()) addresses.add(optional.get());
        }
        return addresses;
    }

    @Override
    public List<com.breadwallet.crypto.Address> getOutputs() {
        List<com.breadwallet.crypto.Address> addresses = new ArrayList<>();
        for (BRTxOutput output: coreTransfer.getOutputs()) {
            String addressStr = output.getAddressAsString();
            Optional<Address> optional = Address.createAsBtc(addressStr);
            if (optional.isPresent()) addresses.add(optional.get());
        }
        return addresses;
    }

    @Override
    public com.breadwallet.crypto.Amount getAmount() {
        UnsignedLong fee = coreWallet.getFeeForTx(coreTransfer);
        if (UnsignedLong.MAX_VALUE.equals(fee)) {
            fee = UnsignedLong.ZERO;
        }

        UnsignedLong recv = coreWallet.getAmountReceivedFromTx(coreTransfer);
        UnsignedLong send  = coreWallet.getAmountSentByTx(coreTransfer);

        switch (getDirection()) {
            case RECOVERED:
                return Amount.createAsBtc(send, defaultUnit);
            case SENT:
                return Amount.createAsBtc(send.minus(fee).minus(recv), defaultUnit);
            case RECEIVED:
                return Amount.createAsBtc(recv, defaultUnit);
            default:
                throw new IllegalStateException("Invalid transfer direction");
        }
    }

    @Override
    public com.breadwallet.crypto.Amount getFee() {
        UnsignedLong fee = coreWallet.getFeeForTx(coreTransfer);
        if (UnsignedLong.MAX_VALUE.equals(fee)) {
            fee = UnsignedLong.ZERO;
        }

        switch (getDirection()) {
            case RECOVERED:
                return Amount.createAsBtc(fee, defaultUnit);
            case SENT:
                return Amount.createAsBtc(fee, defaultUnit);
            case RECEIVED:
                return Amount.createAsBtc(UnsignedLong.ZERO, defaultUnit);
            default:
                throw new IllegalStateException("Invalid transfer direction");
        }
    }

    @Override
    public TransferFeeBasis getFeeBasis() {
        // TODO(discuss): There is a comment in the Swift about this; is this OK?
        return TransferFeeBasis.createBtc(BRWallet.DEFAULT_FEE_PER_KB);
    }

    @Override
    public TransferDirection getDirection() {
        UnsignedLong send  = coreWallet.getAmountSentByTx(coreTransfer);
        if (send.equals(UnsignedLong.ZERO)) {
            return TransferDirection.RECEIVED;
        }

        UnsignedLong fee = coreWallet.getFeeForTx(coreTransfer);
        if (UnsignedLong.MAX_VALUE.equals(fee)) {
            fee = UnsignedLong.ZERO;
        }

        UnsignedLong recv = coreWallet.getAmountReceivedFromTx(coreTransfer);

        checkState(send.compareTo(fee) >= 0);
        if (send.minus(fee).equals(recv)) {
            return TransferDirection.RECOVERED;

        } else if (send.minus(fee).compareTo(recv) > 0) {
            return TransferDirection.SENT;

        } else {
            return TransferDirection.RECEIVED;
        }
    }

    @Override
    public Optional<com.breadwallet.crypto.TransferHash> getHash() {
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
