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

import com.google.common.primitives.UnsignedLong;

public final class TransferFeeBasis {

    private final CurrencyTransferFeeBasis impl;

    public static TransferFeeBasis createBtc(UnsignedLong btcFeePerKb) {
        return new TransferFeeBasis(new BitcoinTransferFeeBasis(btcFeePerKb));
    }

    public static TransferFeeBasis createEth(Amount ethGasPrice, UnsignedLong ethGasLimit) {
        return new TransferFeeBasis(new EthereumTransferFeeBasis(ethGasPrice, ethGasLimit));
    }

    private TransferFeeBasis(CurrencyTransferFeeBasis impl) {
        this.impl = impl;
    }

    public UnsignedLong getBtcFeePerKb() {
        return impl.getBtcFeePerKb();
    }

    public Amount getEthGasPrice() {
        return impl.getEthGasPrice();
    }

    public UnsignedLong getEthGasLimit() {
        return impl.getEthGasLimit();
    }

    // TODO(discuss): Should getBtcFeePerKb(), getEthGasPrice(), and getEthGasLimit() be returning Optional instead of throwing an exception?
    private interface CurrencyTransferFeeBasis {

        default UnsignedLong getBtcFeePerKb() {
            throw new IllegalStateException("Invalid transfer fee type");
        }

        default Amount getEthGasPrice() {
            throw new IllegalStateException("Invalid transfer fee type");
        }

        default UnsignedLong getEthGasLimit() {
            throw new IllegalStateException("Invalid transfer fee type");
        }
    }

    private static class BitcoinTransferFeeBasis implements CurrencyTransferFeeBasis {

        private final UnsignedLong btcFeePerKb;

        private BitcoinTransferFeeBasis(UnsignedLong btcFeePerKb) {
            this.btcFeePerKb = btcFeePerKb;
        }

        public UnsignedLong getBtcFeePerKb() {
            return btcFeePerKb;
        }
    }

    private static class EthereumTransferFeeBasis implements CurrencyTransferFeeBasis {

        private final Amount ethGasPrice;
        private final UnsignedLong ethGasLimit;

        private EthereumTransferFeeBasis(Amount ethGasPrice, UnsignedLong ethGasLimit) {
            this.ethGasPrice = ethGasPrice;
            this.ethGasLimit = ethGasLimit;
        }

        public Amount getEthGasPrice() {
            return ethGasPrice;
        }

        public UnsignedLong getEthGasLimit() {
            return ethGasLimit;
        }
    }
}
