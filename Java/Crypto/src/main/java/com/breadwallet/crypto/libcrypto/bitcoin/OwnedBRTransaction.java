/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.bitcoin;

import com.breadwallet.crypto.libcrypto.CryptoLibrary;
import com.breadwallet.crypto.libcrypto.support.UInt256;

/* package */
class OwnedBRTransaction implements CoreBRTransaction {

    private final BRTransaction core;

    /* package */
    OwnedBRTransaction(BRTransaction core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.BRTransactionFree(core);
        }
    }

    @Override
    public BRTxInput[] getInputs() {
        return core.getInputs();
    }

    @Override
    public BRTxOutput[] getOutputs() {
        return core.getOutputs();
    }

    @Override
    public UInt256 getTxHash() {
        return core.getTxHash();
    }

    @Override
    public BRTransaction asBRTransaction() {
        return core;
    }

    @Override
    public BRTransaction asBRTransactionDeepCopy() {
        return core.asBRTransactionDeepCopy();
    }

    @Override
    public byte[] serialize() {
        return core.serialize();
    }
}
