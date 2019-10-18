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


public class BRCryptoCWMClientBtc extends Structure {

    //
    // Implementation Detail
    //

    public interface BRCryptoCWMBtcGetBlockNumberCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState);
    }

    public interface BRCryptoCWMBtcGetTransactionsCallback extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer callbackState,
                      Pointer addrs,
                      SizeT addrCount,
                      long begBlockNumber,
                      long endBlockNumber);
    }

    public interface BRCryptoCWMBtcSubmitTransactionCallback extends Callback {
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

    public interface GetBlockNumberCallback extends BRCryptoCWMBtcGetBlockNumberCallback {
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

    public interface GetTransactionsCallback extends BRCryptoCWMBtcGetTransactionsCallback {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoCWMClientCallbackState callbackState,
                    List<String> addresses,
                    long begBlockNumber,
                    long endBlockNumber);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer callbackState,
                              Pointer addrs,
                              SizeT addrCount,
                              long begBlockNumber,
                              long endBlockNumber) {
            int addressesCount = UnsignedInts.checkedCast(addrCount.longValue());
            String[] addressesArray = addrs.getStringArray(0, addressesCount, "UTF-8");
            List<String> addressesList = Arrays.asList(addressesArray);

            handle(
                    new Cookie(context),
                    new BRCryptoWalletManager(manager),
                    new BRCryptoCWMClientCallbackState(callbackState),
                    addressesList,
                    begBlockNumber,
                    endBlockNumber
            );
        }
    }

    public interface SubmitTransactionCallback extends BRCryptoCWMBtcSubmitTransactionCallback {
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

    public BRCryptoCWMBtcGetBlockNumberCallback funcGetBlockNumber;
    public BRCryptoCWMBtcGetTransactionsCallback funcGetTransactions;
    public BRCryptoCWMBtcSubmitTransactionCallback funcSubmitTransaction;

    public BRCryptoCWMClientBtc() {
        super();
    }

    public BRCryptoCWMClientBtc(Pointer peer) {
        super(peer);
    }

    public BRCryptoCWMClientBtc(GetBlockNumberCallback funcGetBlockNumber,
                                GetTransactionsCallback funcGetTransactions,
                                SubmitTransactionCallback funcSubmitTransaction) {
        super();
        this.funcGetBlockNumber = funcGetBlockNumber;
        this.funcGetTransactions = funcGetTransactions;
        this.funcSubmitTransaction = funcSubmitTransaction;
    }

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("funcGetBlockNumber", "funcGetTransactions", "funcSubmitTransaction");
    }

    public ByValue toByValue() {
        ByValue other = new ByValue();
        other.funcGetBlockNumber = this.funcGetBlockNumber;
        other.funcGetTransactions = this.funcGetTransactions;
        other.funcSubmitTransaction = this.funcSubmitTransaction;
        return other;
    }

    public static class ByReference extends BRCryptoCWMClientBtc implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoCWMClientBtc implements Structure.ByValue {

    }
}
