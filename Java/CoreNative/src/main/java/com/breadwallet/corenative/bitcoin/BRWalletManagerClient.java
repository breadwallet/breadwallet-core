/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.bitcoin;

import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import java.util.Arrays;
import java.util.List;

public class BRWalletManagerClient extends Structure {

    public interface BRGetBlockNumberCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, int rid);
    }

    public interface BRGetTransactionsCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, long begBlockNumber, long endBlockNumber, int rid);
    }

    public interface BRSubmitTransactionCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, BRWallet wallet, BRTransaction transaction, int rid);
    }

    public interface BRTransactionEventCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, BRWallet wallet, BRTransaction transaction, BRTransactionEvent.ByValue event);
    }

    public interface BRWalletEventCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, BRWallet wallet, BRWalletEvent.ByValue event);
    }

    public interface BRWalletManagerEventCallback extends Callback {
        void apply(Pointer context, BRWalletManager manager, BRWalletManagerEvent.ByValue event);
    }

    public Pointer context;
    public BRGetBlockNumberCallback funcGetBlockNumber;
    public BRGetTransactionsCallback funcGetTransactions;
    public BRSubmitTransactionCallback funcSubmitTransaction;
    public BRTransactionEventCallback funcTransactionEvent;
    public BRWalletEventCallback funcWalletEvent;
    public BRWalletManagerEventCallback funcWalletManagerEvent;
    public BRWalletManagerClient() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("context", "funcGetBlockNumber", "funcGetTransactions", "funcSubmitTransaction", "funcTransactionEvent", "funcWalletEvent", "funcWalletManagerEvent");
    }

    public BRWalletManagerClient(Pointer context, BRGetBlockNumberCallback funcGetBlockNumber, BRGetTransactionsCallback funcGetTransactions, BRSubmitTransactionCallback funcSubmitTransaction, BRTransactionEventCallback funcTransactionEvent, BRWalletEventCallback funcWalletEvent, BRWalletManagerEventCallback funcWalletManagerEvent) {
        super();
        this.context = context;
        this.funcGetBlockNumber = funcGetBlockNumber;
        this.funcGetTransactions = funcGetTransactions;
        this.funcSubmitTransaction = funcSubmitTransaction;
        this.funcTransactionEvent = funcTransactionEvent;
        this.funcWalletEvent = funcWalletEvent;
        this.funcWalletManagerEvent = funcWalletManagerEvent;
    }

    public BRWalletManagerClient(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRWalletManagerClient implements Structure.ByReference {

    }

    public static class ByValue extends BRWalletManagerClient implements Structure.ByValue {

        public ByValue(Pointer context, BRGetBlockNumberCallback funcGetBlockNumber, BRGetTransactionsCallback funcGetTransactions, BRSubmitTransactionCallback funcSubmitTransaction, BRTransactionEventCallback funcTransactionEvent, BRWalletEventCallback funcWalletEvent, BRWalletManagerEventCallback funcWalletManagerEvent) {
            super(context, funcGetBlockNumber, funcGetTransactions, funcSubmitTransaction, funcTransactionEvent, funcWalletEvent, funcWalletManagerEvent);
        }
    }
}
