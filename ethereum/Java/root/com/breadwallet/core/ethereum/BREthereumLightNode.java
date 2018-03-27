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

/**
 *
 */
public class BREthereumLightNode extends BRCoreJniReference {

    //
    // Client
    //
    interface Client {
        void assignNode (BREthereumLightNode node);
    }

    public interface ClientLES extends Client {
    }

    public interface ClientJSON_RPC extends Client {
        // typedef const char* (*JsonRpcGetBalance) (JsonRpcContext context, int id, String account);
        String getBalance(int id, String account);

        // typedef const char* (*JsonRpcGetGasPrice) (JsonRpcContext context, int id);
        String getGasPrice(int id);

        // typedef const char* (*JsonRpcEstimateGas) (JsonRpcContext context, int id, String to, String amount, String data);
        String getGasEstimate(int id, String to, String amount, String data);

        // typedef const char* (*JsonRpcSubmitTransaction) (JsonRpcContext context, int id, String transaction);
        String submitTransaction(int id, String rawTransaction);

        // typedef void (*JsonRpcGetTransactions) (JsonRpcContext context, int id, String account,
        //                                         JsonRpcAnnounceTransaction announceTransaction,
        //                                         JsonRpcAnnounceTransactionContext announceTransactionContext);
        void getTransactions(int id, String account);
    }

    //
    // Light Node Types
    //
    static public class LES extends BREthereumLightNode {
        public LES(ClientLES client, BREthereumNetwork network, String paperKey) {
            super(BREthereumLightNode.jniCreateLightNodeLES(client, network.getIdentifier(), paperKey),
                    client,
                    network);
        }
    }

    static public class JSON_RPC extends BREthereumLightNode {
        public JSON_RPC(ClientJSON_RPC client, BREthereumNetwork network, String paperKey) {
            super(BREthereumLightNode.jniCreateLightNodeJSON_RPC(client, network.getIdentifier(), paperKey),
                    client,
                    network);
        }
    }

    //
    // Light Node
    //
    WeakReference<Client> client;
    BREthereumNetwork network;
    BREthereumAccount account;

    protected BREthereumLightNode(long jniReferenceAddress, Client client, BREthereumNetwork network) {
        super(jniReferenceAddress);
        this.client = new WeakReference<Client>(client);
        this.network = network;
        this.account = new BREthereumAccount(this, jniLightNodeGetAccount());
        client.assignNode(this);
    }

    //
    // Account
    //
    public BREthereumAccount getAccount() {
        return account;
    }

    public String getAddress () {
        return account.getPrimaryAddress();
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

    //
    // Wallet
    //
    public BREthereumWallet getWallet () {
        return new BREthereumWallet(this,
                jniLightNodeGetWallet(),
                getAccount(),
                network);
    }

    public BREthereumWallet getWallet(BREthereumToken token) {
        long walletId = jniLightNodeGetWalletToken(token.getIdentifier());
        return 0 == walletId
                ? null
                : new BREthereumWallet(this, walletId, getAccount(), network, token);
    }

    public BREthereumWallet createWallet(BREthereumToken token) {
        return new BREthereumWallet(this,
                jniLightNodeCreateWalletToken(token.getIdentifier()),
                getAccount(),
                network,
                token);
    }

    //
    // Transaction
    //
    public void forceTransactionUpdate () {
        jniForceTransactionUpdate();
    }

    public void announceTransaction(String hash,
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
        jniAnnounceTransaction(hash, from, to, contract, amount, gasLimit, gasPrice, data, nonce, gasUsed,
                blockNumber, blockHash, blockConfirmations, blockTransactionIndex, blockTimestamp,
                isError);
    }


    //
    // JNI Interface
    //

    protected static native long jniCreateLightNodeLES(Client client, long network, String paperKey);

    protected static native long jniCreateLightNodeJSON_RPC(Client client, long network, String paperKey);

    protected native long jniLightNodeGetAccount();
    protected native long jniLightNodeGetWallet();
    protected native long jniLightNodeGetWalletToken (long tokenId);

    protected native long jniLightNodeCreateWalletToken(long tokenId);

    protected native String jniGetAccountPrimaryAddress(long acountId);
    protected native String jniGetWalletBalance (long walletId, long unit);
    protected native void jniEstimateWalletGasPrice (long walletId);

    protected native void jniForceWalletBalanceUpdate(long wallet);

    protected native void jniForceTransactionUpdate();

    protected native void jniAnnounceTransaction(String hash,
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

    protected native long jniCreateTransaction (long walletId,
                                                String to,
                                                String amount,
                                                long amountUnit);

    protected native void jniSignTransaction (long walletId,
                                              long transactionId,
                                              String paperKey);

    protected native void jniSubmitTransaction (long walletId,
                                                long transactionId);

    protected native long[] jniGetTransactions (long walletId);

    protected native String[] jniGetTransactionProperties (long transactionId,
                                                           long properties[]);

    protected native String jniGetTransactionAmount (long transactionId, long unit);
    protected native boolean jniTransactionHasToken (long transactionId);

    protected native void jniEstimateTransactionGas (long walletId,
                                                     long transactionId);

    protected native boolean jniLightNodeConnect ();
    protected native boolean jniLightNodeDisconnect ();

    //    public native void disposeNative ();

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

