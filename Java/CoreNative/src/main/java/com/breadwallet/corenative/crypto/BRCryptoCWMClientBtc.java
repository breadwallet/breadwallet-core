/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
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

    public interface BRCryptoCWMBtcGetBlockNumberCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState);
    }

    public interface BRCryptoCWMBtcGetTransactionsCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState, Pointer addrs,
                   SizeT addrCount, long begBlockNumber, long endBlockNumber);
    }

    public interface BRCryptoCWMBtcSubmitTransactionCallback extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoCWMClientCallbackState callbackState, Pointer tx,
                   SizeT txLength, String hashAsHex);
    }

    public BRCryptoCWMBtcGetBlockNumberCallback funcGetBlockNumber;
    public BRCryptoCWMBtcGetTransactionsCallback funcGetTransactions;
    public BRCryptoCWMBtcSubmitTransactionCallback funcSubmitTransaction;

    public BRCryptoCWMClientBtc() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("funcGetBlockNumber", "funcGetTransactions", "funcSubmitTransaction");
    }

    public BRCryptoCWMClientBtc(BRCryptoCWMBtcGetBlockNumberCallback funcGetBlockNumber,
                                BRCryptoCWMBtcGetTransactionsCallback funcGetTransactions,
                                BRCryptoCWMBtcSubmitTransactionCallback funcSubmitTransaction) {
        super();
        this.funcGetBlockNumber = funcGetBlockNumber;
        this.funcGetTransactions = funcGetTransactions;
        this.funcSubmitTransaction = funcSubmitTransaction;
    }

    public BRCryptoCWMClientBtc(Pointer peer) {
        super(peer);
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
