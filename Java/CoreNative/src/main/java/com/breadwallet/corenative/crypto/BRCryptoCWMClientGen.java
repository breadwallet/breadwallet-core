/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.utility.SizeT;
import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;


public class BRCryptoCWMClientGen extends Structure {

    public interface BRCryptoCWMGenGetBlockNumberCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState);
    }

    public interface BRCryptoCWMGenGetTransactionsCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                   String address, long begBlockNumber, long endBlockNumber);
    }

    public interface BRCryptoCWMGenSubmitTransactionCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState,
                   Pointer tx, SizeT txLength, String hashAsHex);
    }

    public BRCryptoCWMGenGetBlockNumberCallback funcGetBlockNumber;
    public BRCryptoCWMGenGetTransactionsCallback funcGetTransactions;
    public BRCryptoCWMGenSubmitTransactionCallback funcSubmitTransaction;

    public BRCryptoCWMClientGen() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("funcGetBlockNumber", "funcGetTransactions", "funcSubmitTransaction");
    }

    public BRCryptoCWMClientGen(BRCryptoCWMGenGetBlockNumberCallback funcGetBlockNumber,
                                BRCryptoCWMGenGetTransactionsCallback funcGetTransactions,
                                BRCryptoCWMGenSubmitTransactionCallback funcSubmitTransaction) {
        super();
        this.funcGetBlockNumber = funcGetBlockNumber;
        this.funcGetTransactions = funcGetTransactions;
        this.funcSubmitTransaction = funcSubmitTransaction;
    }

    public BRCryptoCWMClientGen(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRCryptoCWMClientGen implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoCWMClientGen implements Structure.ByValue {

    }
}
