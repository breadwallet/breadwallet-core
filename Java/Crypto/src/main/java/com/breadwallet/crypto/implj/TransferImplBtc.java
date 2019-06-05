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

import java.util.ArrayList;
import java.util.List;

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
        boolean sent = !UnsignedLong.MAX_VALUE.equals(coreWallet.getFeeForTx(coreTransfer));

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
        boolean sent = !UnsignedLong.MAX_VALUE.equals(coreWallet.getFeeForTx(coreTransfer));

        for (BRTxOutput output: coreTransfer.getOutputs()) {
            String addressStr = output.getAddressAsString();
            if (!sent == coreWallet.containsAddress(addressStr)) {
                return AddressImpl.createAsBtc(addressStr).transform(a -> a);
            }
        }
        return Optional.absent();
    }

    @Override
    public List<Address> getSources() {
        boolean sent = !UnsignedLong.MAX_VALUE.equals(coreWallet.getFeeForTx(coreTransfer));

        List<Address> addresses = new ArrayList<>();
        for (BRTxInput input: coreTransfer.getInputs()) {
            String addressStr = input.getAddressAsString();
            if (sent == coreWallet.containsAddress(addressStr)) {
                Optional<AddressImpl> optional = AddressImpl.createAsBtc(addressStr);
                if (optional.isPresent()) addresses.add(optional.get());
            }
        }
        return addresses;
    }

    @Override
    public List<Address> getTargets() {
        boolean sent = !UnsignedLong.MAX_VALUE.equals(coreWallet.getFeeForTx(coreTransfer));

        List<Address> addresses = new ArrayList<>();
        for (BRTxOutput output: coreTransfer.getOutputs()) {
            String addressStr = output.getAddressAsString();
            if (!sent == coreWallet.containsAddress(addressStr)) {
                Optional<AddressImpl> optional = AddressImpl.createAsBtc(addressStr);
                if (optional.isPresent()) addresses.add(optional.get());
            }
        }
        return addresses;
    }

    @Override
    public List<Address> getInputs() {
        List<Address> addresses = new ArrayList<>();
        for (BRTxInput input: coreTransfer.getInputs()) {
            String addressStr = input.getAddressAsString();
            Optional<AddressImpl> optional = AddressImpl.createAsBtc(addressStr);
            if (optional.isPresent()) addresses.add(optional.get());
        }
        return addresses;
    }

    @Override
    public List<Address> getOutputs() {
        List<Address> addresses = new ArrayList<>();
        for (BRTxOutput output: coreTransfer.getOutputs()) {
            String addressStr = output.getAddressAsString();
            Optional<AddressImpl> optional = AddressImpl.createAsBtc(addressStr);
            if (optional.isPresent()) addresses.add(optional.get());
        }
        return addresses;
    }

    @Override
    public Amount getAmount() {
        UnsignedLong fee = coreWallet.getFeeForTx(coreTransfer);
        if (UnsignedLong.MAX_VALUE.equals(fee)) {
            fee = UnsignedLong.ZERO;
        }

        UnsignedLong recv = coreWallet.getAmountReceivedFromTx(coreTransfer);
        UnsignedLong send  = coreWallet.getAmountSentByTx(coreTransfer);

        switch (getDirection()) {
            case RECOVERED:
                return AmountImpl.createAsBtc(send, defaultUnit);
            case SENT:
                return AmountImpl.createAsBtc(send.minus(recv).minus(fee), defaultUnit);
            case RECEIVED:
                return AmountImpl.createAsBtc(recv, defaultUnit);
            default:
                throw new IllegalStateException("Invalid transfer direction");
        }
    }

    @Override
    public Amount getFee() {
        UnsignedLong fee = coreWallet.getFeeForTx(coreTransfer);
        if (UnsignedLong.MAX_VALUE.equals(fee)) {
            fee = UnsignedLong.ZERO;
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
        UnsignedLong fee = coreWallet.getFeeForTx(coreTransfer);
        if (UnsignedLong.MAX_VALUE.equals(fee)) {
            fee = UnsignedLong.ZERO;
        }

        UnsignedLong recv = coreWallet.getAmountReceivedFromTx(coreTransfer);
        UnsignedLong send  = coreWallet.getAmountSentByTx(coreTransfer);

        if (send.compareTo(UnsignedLong.ZERO) > 0 && (recv.plus(fee)).equals(send)) {
            return TransferDirection.RECOVERED;

        } else if (send.compareTo((recv.plus(fee))) > 0) {
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
    CoreBRTransaction getCoreBRTransaction() {
        return coreTransfer;
    }

    /* package */
    boolean matches(BRTransaction transferImpl) {
        return coreTransfer.equals(transferImpl);
    }
}
