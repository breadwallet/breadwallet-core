package com.breadwallet.crypto;

public final class TransferConfirmation {

    public final long blockNumber;
    public final long transactionIndex;
    public final long timestamp;
    public final Amount fee;

    public TransferConfirmation(long blockNumber, long transactionIndex, long timestamp, Amount fee) {
        this.blockNumber = blockNumber;
        this.transactionIndex = transactionIndex;
        this.timestamp = timestamp;
        this.fee = fee;
    }
}
