package com.breadwallet.crypto.transfer;

import com.breadwallet.crypto.Amount;

public final class TransferFeeBasis {

    public final TransferType type;
    public final long btcFeePerKb;
    public final Amount ethGasPrice;
    public final long ethGasLimit;

    public TransferFeeBasis createBtc(long btcFeePerKb) {
        return new TransferFeeBasis(TransferType.BTC, btcFeePerKb, null, 0);
    }

    public TransferFeeBasis createEth(Amount ethGasPrice, long ethGasLimit) {
        return new TransferFeeBasis(TransferType.ETH, 0, ethGasPrice, ethGasLimit);
    }

    private TransferFeeBasis(TransferType type, long btcFeePerKb, Amount ethGasPrice, long ethGasLimit) {
        this.type = type;
        this.btcFeePerKb = btcFeePerKb;
        this.ethGasPrice = ethGasPrice;
        this.ethGasLimit = ethGasLimit;
    }
}
