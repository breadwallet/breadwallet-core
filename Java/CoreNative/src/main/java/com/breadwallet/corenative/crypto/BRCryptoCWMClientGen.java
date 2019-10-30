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


public class BRCryptoCWMClientGen extends Structure {

    //
    // Implementation Detail
    //

    public interface BRCryptoCWMGenGetBlockNumberCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState);
    }

    public interface BRCryptoCWMGenGetTransactionsCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String address,
                      long begBlockNumber,
                      long endBlockNumber);
    }

    public interface BRCryptoCWMGenGetTransfersCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      String address,
                      long begBlockNumber,
                      long endBlockNumber);
    }

    public interface BRCryptoCWMGenSubmitTransactionCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      Pointer tx,
                      SizeT txLength,
                      String hashAsHex);
    }

    //
    // Client Interface
    //

    public interface GetBlockNumberCallback extends BRCryptoCWMGenGetBlockNumberCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState) {
            handle(
                    new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoCWMClientCallbackState(callbackState)
            );
        }
    }

    public interface GetTransactionsCallback extends BRCryptoCWMGenGetTransactionsCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String address,
                    long begBlockNumber,
                    long endBlockNumber);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String address,
                              long begBlockNumber,
                              long endBlockNumber) {
            handle(
                    new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoCWMClientCallbackState(callbackState),
                    address,
                    begBlockNumber,
                    endBlockNumber
            );
        }
    }

    public interface GetTransfersCallback extends BRCryptoCWMGenGetTransfersCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    String address,
                    long begBlockNumber,
                    long endBlockNumber);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              String address,
                              long begBlockNumber,
                              long endBlockNumber) {
            handle(
                    new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoCWMClientCallbackState(callbackState),
                    address,
                    begBlockNumber,
                    endBlockNumber
            );
        }
    }

    public interface SubmitTransactionCallback extends BRCryptoCWMGenSubmitTransactionCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
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
                    new BRCryptoCWMClientCallbackState(callbackState),
                    tx.getByteArray(0, UnsignedInts.checkedCast(txLength.longValue())),
                    hashAsHex
            );
        }
    }

    //
    // Client Struct
    //

    public BRCryptoCWMGenGetBlockNumberCallback funcGetBlockNumber;
    public BRCryptoCWMGenGetTransactionsCallback funcGetTransactions;
    public BRCryptoCWMGenGetTransfersCallback funGetTransfers;
    public BRCryptoCWMGenSubmitTransactionCallback funcSubmitTransaction;

    public BRCryptoCWMClientGen() {
        super();
    }

    public BRCryptoCWMClientGen(Pointer peer) {
        super(peer);
    }

    public BRCryptoCWMClientGen(GetBlockNumberCallback funcGetBlockNumber,
                                GetTransactionsCallback funcGetTransactions,
                                GetTransfersCallback funGetTransfers,
                                SubmitTransactionCallback funcSubmitTransaction) {
        super();
        this.funcGetBlockNumber = funcGetBlockNumber;
        this.funcGetTransactions = funcGetTransactions;
        this.funGetTransfers = funGetTransfers;
        this.funcSubmitTransaction = funcSubmitTransaction;
    }
    protected List<String> getFieldOrder() {
        return Arrays.asList("funcGetBlockNumber", "funcGetTransactions", "funGetTransfers", "funcSubmitTransaction");
    }


    public ByValue toByValue() {
        ByValue other = new ByValue();
        other.funcGetBlockNumber = this.funcGetBlockNumber;
        other.funcGetTransactions = this.funcGetTransactions;
        other.funGetTransfers = this.funGetTransfers;
        other.funcSubmitTransaction = this.funcSubmitTransaction;
        return other;
    }

    public static class ByReference extends BRCryptoCWMClientGen implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoCWMClientGen implements Structure.ByValue {

    }
}
