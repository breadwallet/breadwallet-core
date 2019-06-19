/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
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


public class BRCryptoCWMClientBtc extends Structure {

    public interface BRCryptoCWMGetBlockNumberCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCallbackHandle handle);
    }

    public interface BRCryptoCWMGetTransactionsCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, Pointer addrs,
                   SizeT addrCount, long begBlockNumber, long endBlockNumber);
    }

    public interface BRCryptoCWMSubmitTransactionCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, Pointer tx,
                   SizeT txLength);
    }

    public BRCryptoCWMGetBlockNumberCallback funcGetBlockNumber;
    public BRCryptoCWMGetTransactionsCallback funcGetTransactions;
    public BRCryptoCWMSubmitTransactionCallback funcSubmitTransaction;

    public BRCryptoCWMClientBtc() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("funcGetBlockNumber", "funcGetTransactions", "funcSubmitTransaction");
    }

    public BRCryptoCWMClientBtc(BRCryptoCWMGetBlockNumberCallback funcGetBlockNumber,
                                BRCryptoCWMGetTransactionsCallback funcGetTransactions,
                                BRCryptoCWMSubmitTransactionCallback funcSubmitTransaction) {
        super();
        this.funcGetBlockNumber = funcGetBlockNumber;
        this.funcGetTransactions = funcGetTransactions;
        this.funcSubmitTransaction = funcSubmitTransaction;
    }

    public BRCryptoCWMClientBtc(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRCryptoCWMClientBtc implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoCWMClientBtc implements Structure.ByValue {

    }
}
