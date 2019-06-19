/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.ethereum.BREthereumEwm;
import com.breadwallet.corenative.ethereum.BREthereumTransfer;
import com.breadwallet.corenative.ethereum.BREthereumWallet;
import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BRCryptoCWMClientEth extends Structure {

    public interface BRCryptoCWMEthGetEtherBalanceCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, String networkName, String address);
    }

    public interface BRCryptoCWMEthGetTokenBalanceCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, String networkName, String address, String tokenAddress);
    }

    public interface BREthereumClientHandlerGetGasPrice extends Callback {
        void apply(Pointer context, BREthereumEwm ewm, BREthereumWallet wid, int rid);
    }

    public interface BREthereumClientHandlerEstimateGas extends Callback {
        void apply(Pointer context, BREthereumEwm ewm, BREthereumWallet wid, BREthereumTransfer tid,
                   String from, String to, String amount, String data, int rid);
    }

    public interface BREthereumClientHandlerSubmitTransaction extends Callback {
        void apply(Pointer context, BREthereumEwm ewm, BREthereumWallet wid, BREthereumTransfer tid,
                   String transaction, int rid);
    }

    public interface BREthereumClientHandlerGetTransactions extends Callback {
        void apply(Pointer context, BREthereumEwm ewm, String address, long begBlockNumber, long endBlockNumber,
                   int rid);
    }

    public interface BREthereumClientHandlerGetLogs extends Callback {
        void apply(Pointer context, BREthereumEwm ewm, String contract, String address, String event,
                   long begBlockNumber, long endBlockNumber, int rid);
    }

    public interface BREthereumClientHandlerGetBlocks extends Callback {
        void apply(Pointer context, BREthereumEwm ewm, String address, int interests, long blockNumberStart,
                   long blockNumberStop, int rid);
    }

    public interface BREthereumClientHandlerGetTokens extends Callback {
        void apply(Pointer context, BREthereumEwm ewm, int rid);
    }

    public interface BREthereumClientHandlerGetBlockNumber extends Callback {
        void apply(Pointer context, BREthereumEwm ewm, int rid);
    }

    public interface BREthereumClientHandlerGetNonce extends Callback {
        void apply(Pointer context, BREthereumEwm ewm, String address, int rid);
    }

    public BRCryptoCWMEthGetEtherBalanceCallback funcGetEtherBalance;
    public BRCryptoCWMEthGetTokenBalanceCallback funcGetTokenBalance;
    public BREthereumClientHandlerGetGasPrice funcGetGasPrice;
    public BREthereumClientHandlerEstimateGas funcEstimateGas;
    public BREthereumClientHandlerSubmitTransaction funcSubmitTransaction;
    public BREthereumClientHandlerGetTransactions funcGetTransactions;
    public BREthereumClientHandlerGetLogs funcGetLogs;
    public BREthereumClientHandlerGetBlocks funcGetBlocks;
    public BREthereumClientHandlerGetTokens funcGetTokens;
    public BREthereumClientHandlerGetBlockNumber funcGetBlockNumber;
    public BREthereumClientHandlerGetNonce funcGetNonce;

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
                                BREthereumClientHandlerGetGasPrice funcGetGasPrice,
                                BREthereumClientHandlerEstimateGas funcEstimateGas,
                                BREthereumClientHandlerSubmitTransaction funcSubmitTransaction,
                                BREthereumClientHandlerGetTransactions funcGetTransactions,
                                BREthereumClientHandlerGetLogs funcGetLogs,
                                BREthereumClientHandlerGetBlocks funcGetBlocks,
                                BREthereumClientHandlerGetTokens funcGetTokens,
                                BREthereumClientHandlerGetBlockNumber funcGetBlockNumber,
                                BREthereumClientHandlerGetNonce funcGetNonce) {
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
