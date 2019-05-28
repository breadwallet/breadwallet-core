package com.breadwallet.crypto;

import com.google.common.base.Optional;

public final class TransferFeeBasis {

    private final Impl impl;

    public static TransferFeeBasis createBtc(long btcFeePerKb) {
        return new TransferFeeBasis(new Bitcoin(btcFeePerKb));
    }

    public static TransferFeeBasis createEth(Amount ethGasPrice, long ethGasLimit) {
        return new TransferFeeBasis(new Ethereum(ethGasPrice, ethGasLimit));
    }

    private TransferFeeBasis(Impl impl) {
        this.impl = impl;
    }

    public long getBtcFeePerKb() {
        return impl.getBtcFeePerKb();
    }

    public Amount getEthGasPrice() {
        return impl.getEthGasPrice();
    }

    public Long getEthGasLimit() {
        return impl.getEthGasLimit();
    }

    private interface Impl {

        default long getBtcFeePerKb() {
            throw new IllegalStateException("Invalid transfer fee type");
        }

        default Amount getEthGasPrice() {
            throw new IllegalStateException("Invalid transfer fee type");
        }

        default long getEthGasLimit() {
            throw new IllegalStateException("Invalid transfer fee type");
        }
    }

    private static class Bitcoin implements Impl {

        private final long btcFeePerKb;

        private Bitcoin(long btcFeePerKb) {
            this.btcFeePerKb = btcFeePerKb;
        }

        public long getBtcFeePerKb() {
            return btcFeePerKb;
        }
    }

    private static class Ethereum implements Impl {

        private final Amount ethGasPrice;
        private final long ethGasLimit;

        private Ethereum(Amount ethGasPrice, long ethGasLimit) {
            this.ethGasPrice = ethGasPrice;
            this.ethGasLimit = ethGasLimit;
        }

        public Amount getEthGasPrice() {
            return ethGasPrice;
        }

        public long getEthGasLimit() {
            return ethGasLimit;
        }
    }
}
