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
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedLong;

public interface CoreBRTransaction {

    static Optional<CoreBRTransaction> create(byte[] rawData, int timestamp, int blockHeight) {
        BRTransaction transaction = CryptoLibrary.INSTANCE.BRTransactionParse(rawData, new SizeT(rawData.length));
        if (null != transaction) {
            transaction.timestamp = timestamp;
            transaction.blockHeight = blockHeight;
        }
        return Optional.fromNullable(transaction).transform(OwnedBRTransaction::new);
    }

    static Optional<CoreBRTransaction> create(BRWallet wallet, UnsignedLong amount, String address) {
        BRTransaction transaction = CryptoLibrary.INSTANCE.BRWalletCreateTransaction(wallet, amount.longValue(), address);
        return Optional.fromNullable(transaction).transform(OwnedBRTransaction::new);
    }

    static CoreBRTransaction create(BRTransaction transaction) {
        return new OwnedBRTransaction(transaction);
    }

    BRTxInput[] getInputs();

    BRTxOutput[] getOutputs();

    UInt256 getTxHash();

    BRTransaction asBRTransaction();

    BRTransaction asBRTransactionDeepCopy();

    byte[] serialize();
}
