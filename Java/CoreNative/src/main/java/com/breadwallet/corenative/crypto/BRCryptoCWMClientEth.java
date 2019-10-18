/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.utility.Cookie;
import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BRCryptoCWMClientEth extends Structure {

    //
    // Implementation Detail
    //

    public interface BRCryptoCWMEthGetEtherBalanceCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String networkName,
                      String address);
    }

    public interface BRCryptoCWMEthGetTokenBalanceCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String networkName,
                      String address,
                      String tokenAddress);
    }

    public interface BRCryptoCWMEthGetGasPriceCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String networkName);
    }

    public interface BRCryptoCWMEthEstimateGasCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String networkName,
                      String from,
                      String to,
                      String amount,
                      String gasPrice,
                      String data);
    }

    public interface BRCryptoCWMEthSubmitTransactionCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String networkName,
                      String transaction);
    }

    public interface BRCryptoCWMEthGetTransactionsCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String networkName,
                      String address,
                      long begBlockNumber,
                      long endBlockNumber);
    }

    public interface BRCryptoCWMEthGetLogsCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String networkName,
                      String contract,
                      String address,
                      String event,
                      long begBlockNumber,
                      long endBlockNumber);
    }

    public interface BRCryptoCWMEthGetBlocksCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String networkName,
                      String address,
                      int interests,
                      long blockNumberStart,
                      long blockNumberStop);
    }

    public interface BRCryptoCWMEthGetTokensCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState);
    }

    public interface BRCryptoCWMEthGetBlockNumberCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String networkName);
    }

    public interface BRCryptoCWMEthGetNonceCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String networkName,
                      String address);
    }

    //
    // Client Interface
    //

    public interface GetEtherBalanceCallback extends BRCryptoCWMEthGetEtherBalanceCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String networkName,
                    String address);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName,
                              String address) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState),
                   networkName,
                   address);
        }
    }

    public interface GetTokenBalanceCallback extends BRCryptoCWMEthGetTokenBalanceCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String networkName,
                    String address,
                    String tokenAddress);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName,
                              String address,
                              String tokenAddress) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState),
                   networkName,
                   address,
                   tokenAddress);
        }
    }

    public interface GetGasPriceCallback extends BRCryptoCWMEthGetGasPriceCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String networkName);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState),
                   networkName);
        }
    }

    public interface EstimateGasCallback extends BRCryptoCWMEthEstimateGasCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String networkName,
                    String from,
                    String to,
                    String amount,
                    String gasPrice,
                    String data);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName,
                              String from,
                              String to,
                              String amount,
                              String gasPrice,
                              String data) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState),
                   networkName,
                   from,
                   to,
                   amount,
                   gasPrice,
                   data);
        }
    }

    public interface SubmitTransactionCallback extends BRCryptoCWMEthSubmitTransactionCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String networkName,
                    String transaction);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName,
                              String transaction) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState),
                   networkName,
                   transaction);
        }
    }

    public interface GetTransactionsCallback  extends BRCryptoCWMEthGetTransactionsCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String networkName,
                    String address,
                    long begBlockNumber,
                    long endBlockNumber);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName,
                              String address,
                              long begBlockNumber,
                              long endBlockNumber) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState),
                   networkName,
                   address,
                   begBlockNumber,
                   endBlockNumber);
        }
    }

    public interface GetLogsCallback extends BRCryptoCWMEthGetLogsCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String networkName,
                    String contract,
                    String address,
                    String event,
                    long begBlockNumber,
                    long endBlockNumber);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName,
                              String contract,
                              String address,
                              String event,
                              long begBlockNumber,
                              long endBlockNumber) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState),
                   networkName,
                   contract,
                   address,
                   event,
                   begBlockNumber,
                   endBlockNumber);
        }
    }

    public interface GetBlocksCallback extends BRCryptoCWMEthGetBlocksCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String networkName,
                    String address,
                    int interests,
                    long blockNumberStart,
                    long blockNumberStop);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName,
                              String address,
                              int interests,
                              long blockNumberStart,
                              long blockNumberStop) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState),
                   networkName,
                   address,
                   interests,
                   blockNumberStart,
                   blockNumberStop);
        }
    }

    public interface GetTokensCallback extends BRCryptoCWMEthGetTokensCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState));
        }
    }

    public interface GetBlockNumberCallback extends BRCryptoCWMEthGetBlockNumberCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String networkName);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState),
                   networkName);
        }
    }

    public interface GetNonceCallback extends BRCryptoCWMEthGetNonceCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String networkName,
                    String address);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName,
                              String address) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoCWMClientCallbackState(callbackState),
                   networkName,
                   address);
        }
    }

    //
    // Client Struct
    //

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

    public BRCryptoCWMClientEth(Pointer peer) {
        super(peer);
    }

    public BRCryptoCWMClientEth(GetEtherBalanceCallback funcGetEtherBalance,
                                GetTokenBalanceCallback funcGetTokenBalance,
                                GetGasPriceCallback funcGetGasPrice,
                                EstimateGasCallback funcEstimateGas,
                                SubmitTransactionCallback funcSubmitTransaction,
                                GetTransactionsCallback funcGetTransactions,
                                GetLogsCallback funcGetLogs,
                                GetBlocksCallback funcGetBlocks,
                                GetTokensCallback funcGetTokens,
                                GetBlockNumberCallback funcGetBlockNumber,
                                GetNonceCallback funcGetNonce) {
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

    protected List<String> getFieldOrder() {
        return Arrays.asList("funcGetEtherBalance", "funcGetTokenBalance", "funcGetGasPrice", "funcEstimateGas", "funcSubmitTransaction",
                "funcGetTransactions", "funcGetLogs", "funcGetBlocks", "funcGetTokens", "funcGetBlockNumber",
                "funcGetNonce");
    }

    public BRCryptoCWMClientEth toByValue() {
        ByValue other = new ByValue();
        other.funcGetEtherBalance = this.funcGetEtherBalance;
        other.funcGetTokenBalance = this.funcGetTokenBalance;
        other.funcGetGasPrice = this.funcGetGasPrice;
        other.funcEstimateGas = this.funcEstimateGas;
        other.funcSubmitTransaction = this.funcSubmitTransaction;
        other.funcGetTransactions = this.funcGetTransactions;
        other.funcGetLogs = this.funcGetLogs;
        other.funcGetBlocks = this.funcGetBlocks;
        other.funcGetTokens = this.funcGetTokens;
        other.funcGetBlockNumber = this.funcGetBlockNumber;
        other.funcGetNonce = this.funcGetNonce;
        return other;
    }

    public static class ByReference extends BRCryptoCWMClientEth implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoCWMClientEth implements Structure.ByValue {

    }
}
