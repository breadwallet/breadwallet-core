/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.utility.Cookie;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.primitives.UnsignedInts;
import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;


public class BRCryptoClient extends Structure {

    //
    // Implementation Detail
    //

    public interface BRCryptoClientGetBalanceCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      Pointer addrs,
                      SizeT addrCount,
                      String issuer);
    }

    public interface BRCryptoClientGetBlockNumberCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState);
    }

    public interface BRCryptoClientGetTransactionsCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      Pointer addrs,
                      SizeT addrCount,
                      String currency,
                      long begBlockNumber,
                      long endBlockNumber);
    }

    public interface BRCryptoClientGetTransfersCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      Pointer addrs,
                      SizeT addrCount,
                      String currency,
                      long begBlockNumber,
                      long endBlockNumber);
    }


    public interface BRCryptoClientSubmitTransactionCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      Pointer tx,
                      SizeT txLength,
                      String hashAsHex);
    }

    // Implementation Detail (ETH)

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

    public interface GetBalanceCallback extends BRCryptoClientGetBalanceCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoClientCallbackState callbackState,
                    List<String> addresses,
                    String issuer);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              Pointer addrs,
                              SizeT addrCount,
                              String issuer) {
            int addressesCount = UnsignedInts.checkedCast(addrCount.longValue());
            String[] addressesArray = addrs.getStringArray(0, addressesCount, "UTF-8");
            List<String> addressesList = Arrays.asList(addressesArray);

            handle(
                    new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoClientCallbackState(callbackState),
                    addressesList,
                    issuer
            );
        }
    }

    public interface GetBlockNumberCallback extends BRCryptoClientGetBlockNumberCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoClientCallbackState callbackState);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState) {
            handle(
                    new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoClientCallbackState(callbackState)
            );
        }
    }

    public interface GetTransactionsCallback extends BRCryptoClientGetTransactionsCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoClientCallbackState callbackState,
                    List<String> addresses,
                    String currency,
                    long begBlockNumber,
                    long endBlockNumber);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              Pointer addrs,
                              SizeT addrCount,
                              String currency,
                              long begBlockNumber,
                              long endBlockNumber) {
            int addressesCount = UnsignedInts.checkedCast(addrCount.longValue());
            String[] addressesArray = addrs.getStringArray(0, addressesCount, "UTF-8");
            List<String> addressesList = Arrays.asList(addressesArray);

            handle(
                    new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoClientCallbackState(callbackState),
                    addressesList,
                    currency,
                    begBlockNumber,
                    endBlockNumber
            );
        }
    }

    public interface GetTransfersCallback extends BRCryptoClientGetTransfersCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoClientCallbackState callbackState,
                    List<String> addresses,
                    String currency,
                    long begBlockNumber,
                    long endBlockNumber);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              Pointer addrs,
                              SizeT addrCount,
                              String currency,
                              long begBlockNumber,
                              long endBlockNumber) {
            int addressesCount = UnsignedInts.checkedCast(addrCount.longValue());
            String[] addressesArray = addrs.getStringArray(0, addressesCount, "UTF-8");
            List<String> addressesList = Arrays.asList(addressesArray);

            handle(
                    new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoClientCallbackState(callbackState),
                    addressesList,
                    currency,
                    begBlockNumber,
                    endBlockNumber
            );
        }
    }

    public interface SubmitTransactionCallback extends BRCryptoClientSubmitTransactionCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoClientCallbackState callbackState,
                    byte[] transaction,
                    String hashAsHex);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              Pointer tx,
                              SizeT txLength,
                              String hashAsHex) {
            handle(
                    new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoClientCallbackState(callbackState),
                    tx.getByteArray(0, UnsignedInts.checkedCast(txLength.longValue())),
                    hashAsHex
            );
        }
    }

    public interface GetGasPriceCallbackETH extends BRCryptoCWMEthGetGasPriceCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoClientCallbackState callbackState,
                    String networkName);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String networkName) {
            handle(new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoClientCallbackState(callbackState),
                    networkName);
        }
    }

    public interface EstimateGasCallbackETH extends BRCryptoCWMEthEstimateGasCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoClientCallbackState callbackState,
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
                    new BRCryptoClientCallbackState(callbackState),
                    networkName,
                    from,
                    to,
                    amount,
                    gasPrice,
                    data);
        }
    }

    public interface GetBlocksCallbackETH extends BRCryptoCWMEthGetBlocksCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoClientCallbackState callbackState,
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
                    new BRCryptoClientCallbackState(callbackState),
                    networkName,
                    address,
                    interests,
                    blockNumberStart,
                    blockNumberStop);
        }
    }

    public interface GetTokensCallbackETH extends BRCryptoCWMEthGetTokensCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoClientCallbackState callbackState);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState) {
            handle(new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoClientCallbackState(callbackState));
        }
    }

    public interface GetNonceCallbackETH extends BRCryptoCWMEthGetNonceCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoClientCallbackState callbackState,
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
                    new BRCryptoClientCallbackState(callbackState),
                    networkName,
                    address);
        }
    }

    //
    // Client Struct
    //

    public Pointer context;

    public BRCryptoClientGetBalanceCallback funcGetBalance;
    public BRCryptoClientGetBlockNumberCallback funcGetBlockNumber;
    public BRCryptoClientGetTransactionsCallback funcGetTransactions;
    public BRCryptoClientGetTransfersCallback funcGetTransfers;
    public BRCryptoClientSubmitTransactionCallback funcSubmitTransaction;

    public BRCryptoCWMEthGetGasPriceCallback funcGetGasPriceETH;
    public BRCryptoCWMEthEstimateGasCallback funcEstimateGasETH;
    public BRCryptoCWMEthGetBlocksCallback funcGetBlocksETH;
    public BRCryptoCWMEthGetTokensCallback funcGetTokensETH;
    public BRCryptoCWMEthGetNonceCallback funcGetNonceETH;

    public BRCryptoClient() {
        super();
    }

    public BRCryptoClient(Pointer peer) {
        super(peer);
    }

    public BRCryptoClient(Cookie context,
                          GetBalanceCallback funcGetBalance,
                          GetBlockNumberCallback funcGetBlockNumber,
                          GetTransactionsCallback funcGetTransactions,
                          GetTransfersCallback funcGetTransfers,
                          SubmitTransactionCallback funcSubmitTransaction,
                          GetGasPriceCallbackETH funcGetGasPriceETH,
                          EstimateGasCallbackETH funcEstimateGasETH,
                          GetBlocksCallbackETH funcGetBlocksETH,
                          GetTokensCallbackETH funcGetTokensETH,
                          GetNonceCallbackETH funcGetNonceETH) {
        super();
        this.context = context.getPointer();
        this.funcGetBalance = funcGetBalance;
        this.funcGetBlockNumber = funcGetBlockNumber;
        this.funcGetTransactions = funcGetTransactions;
        this.funcGetTransfers = funcGetTransfers;
        this.funcSubmitTransaction = funcSubmitTransaction;

        this.funcGetGasPriceETH = funcGetGasPriceETH;
        this.funcEstimateGasETH = funcEstimateGasETH;
        this.funcGetBlocksETH = funcGetBlocksETH;
        this.funcGetTokensETH = funcGetTokensETH;
        this.funcGetNonceETH = funcGetNonceETH;

    }

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList(
                "context",
                "funcGetBalance",
                "funcGetBlockNumber",
                "funcGetTransactions",
                "funcGetTransfers",
                "funcSubmitTransaction",

                "funcGetGasPriceETH",
                "funcEstimateGasETH",
                "funcGetBlocksETH",
                "funcGetTokensETH",
                "funcGetNonceETH"
        );
    }

    public ByValue toByValue() {
        ByValue other = new ByValue();

        other.context = this.context;
        other.funcGetBalance = this.funcGetBalance;
        other.funcGetBlockNumber = this.funcGetBlockNumber;
        other.funcGetTransactions = this.funcGetTransactions;
        other.funcGetTransfers = this.funcGetTransfers;
        other.funcSubmitTransaction = this.funcSubmitTransaction;

        other.funcGetGasPriceETH = this.funcGetGasPriceETH;
        other.funcEstimateGasETH = this.funcEstimateGasETH;
        other.funcGetBlocksETH = this.funcGetBlocksETH;
        other.funcGetTokensETH = this.funcGetTokensETH;
        other.funcGetNonceETH = this.funcGetNonceETH;

        return other;
    }

    public static class ByReference extends BRCryptoClient implements Structure.ByReference {
    }

    public static class ByValue extends BRCryptoClient implements Structure.ByValue {
    }
}
