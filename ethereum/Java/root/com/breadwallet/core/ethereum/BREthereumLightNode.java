/*
 * EthereumLightNode
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 3/7/18.
 * Copyright (c) 2018 breadwallet LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
package com.breadwallet.core.ethereum;

import com.breadwallet.core.BRCoreJniReference;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

/**
 *
 */
public class BREthereumLightNode extends BRCoreJniReference {

    //
    // Client
    //
    public interface Client {
        void assignNode (BREthereumLightNode node);
    }

    public interface ClientLES extends Client {
    }

    public interface ClientJSON_RPC extends Client {
        //        typedef void (*JsonRpcGetBalance) (JsonRpcContext context,
        //                                   BREthereumLightNode node,
        //                                   BREthereumWalletId wid,
        //                                   const char *address,
        //                                   int rid);
        void getBalance(int wid, String address, int rid);

        //        typedef void (*JsonRpcGetGasPrice) (JsonRpcContext context,
        //                                    BREthereumLightNode node,
        //                                    BREthereumWalletId wid,
        //                                    int rid);
        void getGasPrice(int wid, int rid);

        //        typedef void (*JsonRpcEstimateGas) (JsonRpcContext context,
        //                                    BREthereumLightNode node,
        //                                    BREthereumWalletId wid,
        //                                    BREthereumTransactionId tid,
        //                                    const char *to,
        //                                    const char *amount,
        //                                    const char *data,
        //                                    int rid);

        void getGasEstimate(int wid, int tid, String to, String amount, String data, int rid);

        //        typedef void (*JsonRpcSubmitTransaction) (JsonRpcContext context,
        //                                          BREthereumLightNode node,
        //                                          BREthereumWalletId wid,
        //                                          BREthereumTransactionId tid,
        //                                          const char *transaction,
        //                                          int rid);
        void submitTransaction(int wid, int tid, String rawTransaction, int rid);

        //        typedef void (*JsonRpcGetTransactions) (JsonRpcContext context,
        //                                        BREthereumLightNode node,
        //                                        const char *address,
        //                                        int rid);
        void getTransactions(String address, int rid);

        //        typedef void (*JsonRpcGetLogs) (JsonRpcContext context,
        //                                BREthereumLightNode node,
        //                                const char *address,
        //                                const char *event,
        //                                int rid);
        void getLogs (String address, String event, int rid);
    }

    //
    // Light Node LES
    //
    static public class LES extends BREthereumLightNode {
        public LES(ClientLES client, BREthereumNetwork network, String paperKey, String[] wordList) {
            super(BREthereumLightNode.jniCreateLightNodeLES(client, network.getIdentifier(), paperKey, wordList),
                    client,
                    network);
        }

        public LES(ClientLES client, BREthereumNetwork network, byte[] publicKey) {
            super(BREthereumLightNode.jniCreateLightNodeLES_PublicKey(client, network.getIdentifier(), publicKey),
                    client,
                    network);
        }
    }

    //
    // Light Node JSON_RPC
    //
    static public class JSON_RPC extends BREthereumLightNode {
        public JSON_RPC(ClientJSON_RPC client, BREthereumNetwork network, String paperKey, String[] wordList) {
            super(BREthereumLightNode.jniCreateLightNodeJSON_RPC(client, network.getIdentifier(), paperKey, wordList),
                    client,
                    network);
        }

        public JSON_RPC(ClientJSON_RPC client, BREthereumNetwork network, byte[] publicKey) {
            super(BREthereumLightNode.jniCreateLightNodeJSON_RPC_PublicKey(client, network.getIdentifier(), publicKey),
                    client,
                    network);
        }

        public void announceBalance (int wid, String balance, int rid) {
            jniAnnounceBalance(wid, balance, rid);
        }

        public void announceGasPrice (int wid, String gasPrice, int rid) {
            jniAnnounceGasPrice(wid, gasPrice, rid);
        }

        public void announceGasEstimate (int wid, int tid, String gasEstimate, int rid) {
            jniAnnounceGasEstimate(wid, tid, gasEstimate, rid);
        }

        public void announceSubmitTransaction (int wid, int tid, String hash, int rid) {
            jniAnnounceSubmitTransaction(wid, tid, hash, rid);
        }

        public void announceTransaction(int id,
                                        String hash,
                                        String from,
                                        String to,
                                        String contract,
                                        String amount, // value
                                        String gasLimit,
                                        String gasPrice,
                                        String data,
                                        String nonce,
                                        String gasUsed,
                                        String blockNumber,
                                        String blockHash,
                                        String blockConfirmations,
                                        String blockTransactionIndex,
                                        String blockTimestamp,
                                        // cumulative gas used,
                                        // confirmations
                                        // txreceipt_status
                                        String isError) {
            jniAnnounceTransaction(id, hash, from, to, contract, amount, gasLimit, gasPrice, data, nonce, gasUsed,
                    blockNumber, blockHash, blockConfirmations, blockTransactionIndex, blockTimestamp,
                    isError);
        }

        public void announceLog (int id,
                                 String hash,
                                 String contract,
                                 String[] topics,
                                 String data,
                                 String gasPrice,
                                 String gasUsed,
                                 String logIndex,
                                 String blockNumber,
                                 String blockTransactionIndex,
                                 String blockTimestamp) {
            jniAnnounceLog(id, hash, contract, topics, data, gasPrice, gasUsed, logIndex,
                    blockNumber, blockTransactionIndex, blockTimestamp);
        }
    }

    //
    // Listener
    //
    // In the following the Event enumerations *must* match the corresponding declarations in
    // BREthereumLightNode.h - the enumerations values/indices must be identical.
    //
    public interface Listener {
        enum WalletEvent {
            CREATED,
            BALANCE_UPDATED,
            DEFAULT_GAS_LIMIT_UPDATED,
            DEFAULT_GAS_PRICE_UPDATED,
            TRANSACTION_ADDED,
            TRANSACTION_REMOVED,
            DELETED
        }

        void handleWalletEvent (BREthereumWallet wallet, WalletEvent event);

        enum BlockEvent {
            CREATED
        }

        //void handleBlockEvent (BREthereumBlock block, BlockEvent event);

        enum TransactionEvent {
            CREATED,
            SIGNED,
            SUBMITTED,
            BLOCKED,  // aka confirmed
            ERRORED,
            GAS_ESTIMATE_UPDATED
        }

        void handleTransactionEvent (BREthereumWallet wallet, BREthereumTransaction transaction, TransactionEvent event);
    }

    //
    // Light Node
    //
    WeakReference<Client> client;
    WeakReference<Listener> listener;

    //
    // Network
    //
    BREthereumNetwork network;

    public BREthereumNetwork getNetwork () {
        return network;
    }
    //
    // Account
    //
    BREthereumAccount account;

    public BREthereumAccount getAccount() {
        return account;
    }

    public String getAddress () {
        return account.getPrimaryAddress();
    }

    public byte[] getAddressPublicKey () {
        return account.getPrimaryAddressPublicKey();
    }

    //
    // Wallet
    //
    // We hold a mapping, from identifier to wallet, for all wallets held/managed by this node.
    // The Core already holds wallets and thus we don't actually need to 'duplicate' that
    // functionality; however, to not hold wallets here would mean that every getWallet(), every
    // event handler would need to create another Wallet (feeling like a 'value type').  We don't
    // do that - but we could, and might some day.
    //
    // We could hold a WeakReference (and we probably should) - but, at least with the current
    // Android app, we witnessed wallets being reclaimed between each event update.  The consequence
    // was that we re-created the wallets each time; only to have them reclaimed.  Now, that is
    // actually not that big a deal and it should disappear completely when the Android app holds
    // on to wallets that have transactions.
    //
    // Of course, if the wallet shows up, then it is in Core Ethereum, and it shouldn't be
    // a WeakReference() - since it clearly exists in Core.  We'll leave this as a string
    // reference and explicitly delete wallets on a 'DELETE' event.
    //
    protected Map<Long, BREthereumWallet> wallets = new HashMap<>();

    protected BREthereumWallet walletLookupOrCreate(long wid, BREthereumToken token) {
        BREthereumWallet wallet = wallets.get(wid);

        // If we never had a wallet, then create one.
        if (null == wallet) {

            // If `token` is null, then lookup the token for wallet.
            if (null == token) {
                long tokenRef = jniLightNodeWalletGetToken(wid);
                if (0 != tokenRef)
                    token = BREthereumToken.lookupByReference (tokenRef);
            }

            wallet = (null == token
                    ? new BREthereumWallet(this, wid, account, network)
                    : new BREthereumWallet(this, wid, account, network, token));

            wallets.put(wid, wallet);

        }

        return wallet;
    }

    public BREthereumWallet getWallet () {
        long wid = jniLightNodeGetWallet();
        return walletLookupOrCreate (wid, null);
    }

    public BREthereumWallet getWallet(BREthereumToken token) {
        long wid = jniLightNodeGetWalletToken(token.getIdentifier());
        return walletLookupOrCreate(wid, token);
    }

    // TODO: Remove once 'client callbacks' are LightNode trampolines
    public BREthereumWallet getWalletByIdentifier (long wid) {
        return walletLookupOrCreate(wid, null);
    }
    //
    // Transaction
    //
    // We'll hold a mapping, from identifier to transaction, for all transactions.
    //
    protected Map<Long, WeakReference<BREthereumTransaction>> transactions = new HashMap<>();

    protected BREthereumTransaction transactionLookupOrCreate(long tid) {
        WeakReference<BREthereumTransaction> transactionRef = transactions.get(tid);

        if (null == transactionRef || null == transactionRef.get()) {
            long tokenReference = jniTransactionGetToken(tid);

            transactionRef = new WeakReference<>(
                    new BREthereumTransaction(this, tid,
                            (0 == tokenReference
                                    ? BREthereumAmount.Unit.ETHER_ETHER
                                    : BREthereumAmount.Unit.TOKEN_DECIMAL)));
            transactions.put(tid, transactionRef);
        }

        return transactionRef.get();
    }

    //
    // Constructor
    //
    protected BREthereumLightNode(long jniReferenceAddress, Client client, BREthereumNetwork network) {
        super(jniReferenceAddress);

        // `this` is the JNI listener, using the `trampoline` functions to invoke
        // the installed `Listener`.
        jniAddListener(null);

        this.client = new WeakReference<Client>(client);
        this.network = network;
        this.account = new BREthereumAccount(this, jniLightNodeGetAccount());
        client.assignNode(this);
    }

    public void addListener (Listener listener) {
        this.listener = new WeakReference<>(listener);
    }

    protected Listener getListener () {
	return null == listener ? null : listener.get();
    }

    //
    // Connect // Disconnect
    //
    public boolean connect () {
        return jniLightNodeConnect ();
    }
    public boolean disconnect () {
        return jniLightNodeDisconnect ();
    }
    public void disconnectAndWait () {
        jniLightNodeDisconnectAndWait();
    }

    //
    // Callback Announcements
    //
    // In the JNI Code, we had a problem directly accessing the Listener methods for the provided
    // listener (see addListener()).  So instead we'll access these methods below and then 'bounce'
    // to method calls for the listener.
    //
    // These methods also give us a chance to convert the `event`, as a `long`, to the Event.
    //
    protected void trampolineWalletEvent (long wid, long event) {
        Listener l =  getListener();
        if (null == l) return;

        // Lookup the wallet - this will create the wallet if it doesn't exist.  Thus, if the
        // `event` is `create`, we get a wallet; and even, if the `event` is `delete`, we get a
        // wallet too.
        BREthereumWallet wallet = walletLookupOrCreate(wid, null);

        // Invoke handler
        l.handleWalletEvent(wallet, Listener.WalletEvent.values()[(int) event]);
    }

    protected void trampolineBlockEvent (long bid, long event) {
        Listener l = getListener();
        if (null == l) return;

        // Nothing, at this point
    }

    protected void trampolineTransactionEvent (long wid, long tid, long event) {
        Listener l =  getListener();
        if (null == l) return;

        BREthereumWallet wallet = walletLookupOrCreate(wid, null);
        BREthereumTransaction transaction = transactionLookupOrCreate (tid);

        l.handleTransactionEvent(wallet, transaction, Listener.TransactionEvent.values()[(int) event]);
    }

    //
    // JNI: Constructors
    //
    protected static native long jniCreateLightNodeLES(Client client, long network, String paperKey, String[] wordList);
    protected static native long jniCreateLightNodeLES_PublicKey(Client client, long network, byte[] publicKey);

    protected static native long jniCreateLightNodeJSON_RPC(Client client, long network, String paperKey, String[] wordList);
    protected static native long jniCreateLightNodeJSON_RPC_PublicKey(Client client, long network, byte[] publicKey);

    protected native void jniAddListener (Listener listener);

    //
    // JNI: Announcements
    //
    protected native void jniAnnounceTransaction(int id,
                                                 String hash,
                                                 String from,
                                                 String to,
                                                 String contract,
                                                 String amount, // value
                                                 String gasLimit,
                                                 String gasPrice,
                                                 String data,
                                                 String nonce,
                                                 String gasUsed,
                                                 String blockNumber,
                                                 String blockHash,
                                                 String blockConfirmations,
                                                 String blockTransactionIndex,
                                                 String blockTimestamp,
                                                 // cumulative gas used,
                                                 // confirmations
                                                 // txreceipt_status
                                                 String isError);

    protected native void jniAnnounceLog(int id,
                                         String hash,
                                         String contract,
                                         String[] topics,
                                         String data,
                                         String gasPrice,
                                         String gasUsed,
                                         String logIndex,
                                         String blockNumber,
                                         String blockTransactionIndex,
                                         String blockTimestamp);

    protected native void jniAnnounceBalance (int wid, String balance, int rid);
    protected native void jniAnnounceGasPrice (int wid, String gasPrice, int rid);
    protected native void jniAnnounceGasEstimate (int wid, int tid, String gasEstimate, int rid);
    protected native void jniAnnounceSubmitTransaction (int wid, int tid, String hash, int rid);


    // JNI: Account & Address
    protected native long jniLightNodeGetAccount();
    protected native String jniGetAccountPrimaryAddress(long accountId);
    protected native byte[] jniGetAccountPrimaryAddressPublicKey(long accountId);
    protected native byte[] jniGetAccountPrimaryAddressPrivateKey(long accountId, String paperKey);

    // JNI: Wallet
    protected native long jniLightNodeGetWallet();
    protected native long jniLightNodeGetWalletToken (long tokenId);
    protected native long jniLightNodeCreateWalletToken(long tokenId);
    protected native long jniLightNodeWalletGetToken (long wid);

    protected native String jniGetWalletBalance (long walletId, long unit);
    protected native void jniEstimateWalletGasPrice (long walletId);

    protected native void jniForceWalletBalanceUpdate(long wallet);

    protected native long jniWalletGetDefaultGasPrice (long wallet);
    protected native void jniWalletSetDefaultGasPrice (long wallet, long value);

    protected native long jniWalletGetDefaultGasLimit (long wallet);
    protected native void jniWalletSetDefaultGasLimit (long wallet, long value);
    //
    // JNI: Wallet Transactions
    //
    protected native long jniCreateTransaction (long walletId,
                                                String to,
                                                String amount,
                                                long amountUnit);

    protected native void jniSignTransaction (long walletId,
                                              long transactionId,
                                              String paperKey);

    protected native void jniSignTransactionWithPrivateKey(long walletId,
                                                           long transactionId,
                                                           byte[] privateKey);

    protected native void jniSubmitTransaction (long walletId,
                                                long transactionId);

    protected native long[] jniGetTransactions (long walletId);

    protected native void jniTransactionEstimateGas(long walletId,
                                                    long transactionId);

    protected native String jniTransactionEstimateFee (long walletId,
                                                       String amount,
                                                       long amountUnit,
                                                       long resultUnit);

    //
    // JNI: Transactions
    //
    protected native boolean jniTransactionHasToken (long transactionId);

    protected native String jniTransactionGetAmount(long transactionId, long unit);
    protected native String jniTransactionGetFee (long transactionId, long unit);
    protected native String jniTransactionSourceAddress (long transactionId);
    protected native String jniTransactionTargetAddress (long transactionId);
    protected native String jniTransactionGetHash (long transactionId);
    protected native long jniTransactionGetGasPrice (long transactionId);
    protected native long jniTransactionGetGasLimit (long transactionId);
    protected native long jniTransactionGetGasUsed (long transactionId);
    protected native long jniTransactionGetNonce (long transactionId);
    protected native long jniTransactionGetBlockNumber (long transactionId);
    protected native long jniTransactionGetBlockTimestamp (long transactionId);
    protected native long jniTransactionGetToken (long transactionId);
    protected native boolean jniTransactionIsConfirmed (long transactionId);
    protected native boolean jniTransactionIsSubmitted (long transactionId);

    //
    // JNI: Tokens
    //
    protected native String jniTokenGetAddress (long tokenId);

    //
    // JNI: Connect & Disconnect
    //
    protected native boolean jniLightNodeConnect ();
    protected native boolean jniLightNodeDisconnect ();
    protected native void jniLightNodeDisconnectAndWait ();

    // JNI: Initialize
    protected static native void initializeNative();

    static {
        initializeNative();
    }

    //
    // Support
    //

    //
    // Reference
    //
    static class Reference {
        WeakReference<BREthereumLightNode> node;
        long identifier;

        Reference(BREthereumLightNode node, long identifier) {
            this.node = new WeakReference<>(node);
            this.identifier = identifier;
        }
    }

    //
    // Reference With Default Unit
    //
    static class ReferenceWithDefaultUnit extends Reference {
        protected BREthereumAmount.Unit defaultUnit;
        protected boolean defaultUnitUsesToken = false;

        public BREthereumAmount.Unit getDefaultUnit() {
            return defaultUnit;
        }

        public void setDefaultUnit(BREthereumAmount.Unit unit) {
            validUnitOrException(unit);
            this.defaultUnit = unit;
        }

        //
        // Constructor
        //
        protected ReferenceWithDefaultUnit (BREthereumLightNode node,
                                            long identifier,
                                            BREthereumAmount.Unit unit) {
            super(node, identifier);
            this.defaultUnit = unit;
            this.defaultUnitUsesToken = unit.isTokenUnit();
        }

        //
        // Support
        //
        protected boolean validUnit(BREthereumAmount.Unit unit) {
            return (!defaultUnitUsesToken
                    ? (unit == BREthereumAmount.Unit.ETHER_WEI || unit == BREthereumAmount.Unit.ETHER_GWEI || unit == BREthereumAmount.Unit.ETHER_ETHER)
                    : (unit == BREthereumAmount.Unit.TOKEN_DECIMAL || unit == BREthereumAmount.Unit.TOKEN_INTEGER));
        }

        protected void validUnitOrException (BREthereumAmount.Unit unit) {
            if (!validUnit(unit))
                throw new IllegalArgumentException("Invalid Unit for instance type: " + unit.toString());
        }
    }
}
