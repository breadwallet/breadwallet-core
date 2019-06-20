/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BRCryptoCWMClientEth extends Structure {

    public interface BRCryptoCWMEthGetEtherBalanceCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState, String networkName,
                   String address);
    }

    public interface BRCryptoCWMEthGetTokenBalanceCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState, String networkName,
                   String address, String tokenAddress);
    }

    public interface BRCryptoCWMEthGetGasPriceCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState, String networkName);
    }

    public interface BRCryptoCWMEthEstimateGasCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState, String networkName,
                   String from, String to, String amount, String data);
    }

    public interface BRCryptoCWMEthSubmitTransactionCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState, String networkName,
                   String transaction);
    }

    public interface BRCryptoCWMEthGetTransactionsCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState, String networkName,
                   String address, long begBlockNumber, long endBlockNumber);
    }

    public interface BRCryptoCWMEthGetLogsCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState, String networkName,
                   String contract, String address, String event, long begBlockNumber, long endBlockNumber);
    }

    public interface BRCryptoCWMEthGetBlocksCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState, String networkName,
                   String address, int interests, long blockNumberStart, long blockNumberStop);
    }

    public interface BRCryptoCWMEthGetTokensCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState);
    }

    public interface BRCryptoCWMEthGetBlockNumberCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState, String networkName);
    }

    public interface BRCryptoCWMEthGetNonceCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMCompletionState completetionState, String networkName,
                   String address);
    }

    public BRCryptoCWMEthGetEtherBalanceCallback funcGetEtherBalance;
    public BRCryptoCWMEthGetTokenBalanceCallback funcGetTokenBalance;
    public BRCryptoCWMEthGetGasPriceCallback funcGetGasPrice;
    public BRCryptoCWMEthEstimateGasCallback funcEstimateGas;
    public BRCryptoCWMEthSubmitTransactionCallback funcSubmitTransaction;
    public BRCryptoCWMEthGetTransactionsCallback funcGetTransactions;
    public BRCryptoCWMEthGetLogsCallback funcGetLogs;
    public BRCryptoCWMEthGetBlocksCallback funcGetBlocks;
    public BRCryptoCWMEthGetTokensCallback funcGetTokens;
    public BRCryptoCWMEthGetBlockNumberCallback funcGetBlockNumber;
    public BRCryptoCWMEthGetNonceCallback funcGetNonce;

    public BRCryptoCWMClientEth() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("funcGetEtherBalance", "funcGetTokenBalance", "funcGetGasPrice", "funcEstimateGas", "funcSubmitTransaction",
                "funcGetTransactions", "funcGetLogs", "funcGetBlocks", "funcGetTokens", "funcGetBlockNumber",
                "funcGetNonce");
    }

    public BRCryptoCWMClientEth(BRCryptoCWMEthGetEtherBalanceCallback funcGetEtherBalance,
                                BRCryptoCWMEthGetTokenBalanceCallback funcGetTokenBalance,
                                BRCryptoCWMEthGetGasPriceCallback funcGetGasPrice,
                                BRCryptoCWMEthEstimateGasCallback funcEstimateGas,
                                BRCryptoCWMEthSubmitTransactionCallback funcSubmitTransaction,
                                BRCryptoCWMEthGetTransactionsCallback funcGetTransactions,
                                BRCryptoCWMEthGetLogsCallback funcGetLogs,
                                BRCryptoCWMEthGetBlocksCallback funcGetBlocks,
                                BRCryptoCWMEthGetTokensCallback funcGetTokens,
                                BRCryptoCWMEthGetBlockNumberCallback funcGetBlockNumber,
                                BRCryptoCWMEthGetNonceCallback funcGetNonce) {
        super();
        this.funcGetEtherBalance = funcGetEtherBalance;
        this.funcGetTokenBalance = funcGetTokenBalance;
        this.funcGetGasPrice = funcGetGasPrice;
        this.funcEstimateGas = funcEstimateGas;
        this.funcSubmitTransaction = funcSubmitTransaction;
        this.funcGetTransactions = funcGetTransactions;
        this.funcGetLogs = funcGetLogs;
        this.funcGetBlocks = funcGetBlocks;
        this.funcGetTokens = funcGetTokens;
        this.funcGetBlockNumber = funcGetBlockNumber;
        this.funcGetNonce = funcGetNonce;
    }

    public BRCryptoCWMClientEth(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRCryptoCWMClientEth implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoCWMClientEth implements Structure.ByValue {

    }
}
