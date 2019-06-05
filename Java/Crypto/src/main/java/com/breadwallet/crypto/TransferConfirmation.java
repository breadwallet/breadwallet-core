/*
 * TransferConfirmation
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import com.google.common.primitives.UnsignedLong;

import java.util.Date;

public final class TransferConfirmation {

    private static final UnsignedLong SECONDS_MS = UnsignedLong.valueOf(1000);

    private final UnsignedLong blockNumber;
    private final UnsignedLong transactionIndex;
    private final Date timestamp;
    private final Amount fee;

    public TransferConfirmation(UnsignedLong blockNumber, UnsignedLong transactionIndex, UnsignedLong timestamp, Amount fee) {
        this.blockNumber = blockNumber;
        this.transactionIndex = transactionIndex;
        this.timestamp = new Date(timestamp.times(SECONDS_MS).longValue());
        this.fee = fee;
    }

    public UnsignedLong getBlockNumber() {
        return blockNumber;
    }

    public UnsignedLong getTransactionIndex() {
        return transactionIndex;
    }

    public Date getConfirmationTime() {
        return timestamp;
    }

    public Amount getFee() {
        return fee;
    }
}
