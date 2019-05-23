package com.breadwallet.crypto;

import com.google.common.base.Optional;

import javax.annotation.Nullable;

public final class TransferFeeBasis {

    private final TransferType type;

    @Nullable
    private final Long btcFeePerKb;
    @Nullable
    private final Amount ethGasPrice;
    @Nullable
    private final Long ethGasLimit;

    public static TransferFeeBasis createBtc(long btcFeePerKb) {
        return new TransferFeeBasis(TransferType.BTC, btcFeePerKb, null, 0);
    }

    public static TransferFeeBasis createEth(Amount ethGasPrice, long ethGasLimit) {
        return new TransferFeeBasis(TransferType.ETH, 0, ethGasPrice, ethGasLimit);
    }

    private TransferFeeBasis(TransferType type, long btcFeePerKb, Amount ethGasPrice, long ethGasLimit) {
        this.type = type;
        this.btcFeePerKb = btcFeePerKb;
        this.ethGasPrice = ethGasPrice;
        this.ethGasLimit = ethGasLimit;
    }

    public TransferType getType() {
        return type;
    }

    public Optional<Long> getBtcFeePerKb() {
        return Optional.fromNullable(btcFeePerKb);
    }

    public Optional<Amount> getEthGasPrice() {
        return Optional.fromNullable(ethGasPrice);
    }

    public Optional<Long> getEthGasLimit() {
        return Optional.fromNullable(ethGasLimit);
    }
}
