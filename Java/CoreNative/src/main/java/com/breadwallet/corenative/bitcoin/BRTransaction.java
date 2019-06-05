/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.bitcoin;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.support.UInt256;
import com.google.common.primitives.UnsignedInts;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import java.util.Arrays;
import java.util.List;

import static com.google.common.base.Preconditions.checkState;

public class BRTransaction extends Structure implements CoreBRTransaction {

    public UInt256 txHash;
    public UInt256 wtxHash;
    public int version;
    public BRTxInput.ByReference inputs;
    public SizeT inCount;
    public BRTxOutput.ByReference outputs;
    public SizeT outCount;
    public int lockTime;
    public int blockHeight;
    public int timestamp;

    protected List<String> getFieldOrder() {
        return Arrays.asList("txHash", "wtxHash", "version", "inputs", "inCount", "outputs", "outCount", "lockTime", "blockHeight", "timestamp");
    }

    public BRTransaction() {
        super();
    }

    public BRTransaction(Pointer peer) {
        super(peer);
    }

    @Override
    public BRTxInput[] getInputs() {
        return (BRTxInput[]) inputs.toArray(inCount.intValue());
    }

    @Override
    public BRTxOutput[] getOutputs() {
        return (BRTxOutput[]) outputs.toArray(outCount.intValue());
    }

    @Override
    public UInt256 getTxHash() {
        return txHash;
    }

    @Override
    public BRTransaction asBRTransaction() {
        return this;
    }

    @Override
    public BRTransaction asBRTransactionDeepCopy() {
        return CryptoLibrary.INSTANCE.BRTransactionCopy(this);
    }

    @Override
    public byte[] serialize() {
        SizeT sizeNeeded = new SizeT(0);
        sizeNeeded = CryptoLibrary.INSTANCE.BRTransactionSerialize(this, null, sizeNeeded);

        byte[] serialized = new byte[UnsignedInts.checkedCast(sizeNeeded.longValue())];
        CryptoLibrary.INSTANCE.BRTransactionSerialize(this, serialized, sizeNeeded);
        return serialized;
    }

    public static class ByReference extends BRTransaction implements Structure.ByReference {

    }

    public static class ByValue extends BRTransaction implements Structure.ByValue {

    }
}
