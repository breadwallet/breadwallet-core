package com.breadwallet.crypto;

public final class TransferConfirmation {

    private final long blockNumber;
    private final long transactionIndex;
    private final long timestamp;
    private final Amount fee;

    public TransferConfirmation(long blockNumber, long transactionIndex, long timestamp, Amount fee) {
        this.blockNumber = blockNumber;
        this.transactionIndex = transactionIndex;
        this.timestamp = timestamp;
        this.fee = fee;
    }

    public long getBlockNumber() {
        return blockNumber;
    }

    public long getTransactionIndex() {
        return transactionIndex;
    }

    public long getTimestamp() {
        return timestamp;
    }

    public Amount getFee() {
        return fee;
    }
}
