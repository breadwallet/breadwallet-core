/*
 * TransferFeeBasis
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

public final class TransferFeeBasis {

    private final CurrencyTransferFeeBasis impl;

    public static TransferFeeBasis createBtc(long btcFeePerKb) {
        return new TransferFeeBasis(new BitcoinTransferFeeBasis(btcFeePerKb));
    }

    public static TransferFeeBasis createEth(Amount ethGasPrice, long ethGasLimit) {
        return new TransferFeeBasis(new EthereumTransferFeeBasis(ethGasPrice, ethGasLimit));
    }

    private TransferFeeBasis(CurrencyTransferFeeBasis impl) {
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

    // TODO(discuss): Should getBtcFeePerKb(), getEthGasPrice(), and getEthGasLimit() be returning Optional instead of throwing an exception?
    private interface CurrencyTransferFeeBasis {

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

    private static class BitcoinTransferFeeBasis implements CurrencyTransferFeeBasis {

        private final long btcFeePerKb;

        private BitcoinTransferFeeBasis(long btcFeePerKb) {
            this.btcFeePerKb = btcFeePerKb;
        }

        public long getBtcFeePerKb() {
            return btcFeePerKb;
        }
    }

    private static class EthereumTransferFeeBasis implements CurrencyTransferFeeBasis {

        private final Amount ethGasPrice;
        private final long ethGasLimit;

        private EthereumTransferFeeBasis(Amount ethGasPrice, long ethGasLimit) {
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
