/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.bitcoin.BRTransaction;
import com.breadwallet.corenative.bitcoin.BRWallet;
import com.breadwallet.corenative.bitcoin.BRWalletManager;
import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import java.util.Arrays;
import java.util.List;


public class BRCryptoCWMClientBtc extends Structure {

	public interface BRGetBlockNumberCallback extends Callback {
		void apply(Pointer context, BRWalletManager manager, int rid);
	}

	public interface BRGetTransactionsCallback extends Callback {
		void apply(Pointer context, BRWalletManager manager, long begBlockNumber, long endBlockNumber, int rid);
	}

	public interface BRSubmitTransactionCallback extends Callback {
		void apply(Pointer context, BRWalletManager manager, BRWallet wallet, BRTransaction transaction, int rid);
	}

	public BRGetBlockNumberCallback funcGetBlockNumber;
	public BRGetTransactionsCallback funcGetTransactions;
	public BRSubmitTransactionCallback funcSubmitTransaction;

	public BRCryptoCWMClientBtc() {
		super();
	}

	protected List<String> getFieldOrder() {
		return Arrays.asList("funcGetBlockNumber", "funcGetTransactions", "funcSubmitTransaction");
	}

	public BRCryptoCWMClientBtc(BRGetBlockNumberCallback funcGetBlockNumber, BRGetTransactionsCallback funcGetTransactions, BRSubmitTransactionCallback funcSubmitTransaction) {
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
