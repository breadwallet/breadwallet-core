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
    public interface Client {
        void assignNode (BREthereumLightNode node);
    }

    public interface ClientLES extends Client {
    }

    public interface ClientJSON_RPC extends Client {
        //        typedef void (*JsonRpcGetBalance) (JsonRpcContext context,
        //                                   BREthereumLightNode node,
        //                                   BREthereumLightNodeWalletId wid,
        //                                   const char *address,
        //                                   int rid);
        void getBalance(int wid, String address, int rid);

        //        typedef void (*JsonRpcGetGasPrice) (JsonRpcContext context,
        //                                    BREthereumLightNode node,
        //                                    BREthereumLightNodeWalletId wid,
        //                                    int rid);
        void getGasPrice(int wid, int rid);

        //        typedef void (*JsonRpcEstimateGas) (JsonRpcContext context,
        //                                    BREthereumLightNode node,
        //                                    BREthereumLightNodeWalletId wid,
        //                                    BREthereumLightNodeTransactionId tid,
        //                                    const char *to,
        //                                    const char *amount,
        //                                    const char *data,
        //                                    int rid);

        void getGasEstimate(int wid, int tid, String to, String amount, String data, int rid);

        //        typedef void (*JsonRpcSubmitTransaction) (JsonRpcContext context,
        //                                          BREthereumLightNode node,
        //                                          BREthereumLightNodeWalletId wid,
        //                                          BREthereumLightNodeTransactionId tid,
        //                                          const char *transaction,
        //                                          int rid);
        void submitTransaction(int wid, int tid, String rawTransaction, int rid);

        //        typedef void (*JsonRpcGetTransactions) (JsonRpcContext context,
        //                                        BREthereumLightNode node,
        //                                        const char *address,
        //                                        int rid);
        void getTransactions(String address, int rid);
    }

    //
    // Light Node LES
    //
    static public class LES extends BREthereumLightNode {
        public LES(ClientLES client, BREthereumNetwork network, String paperKey) {
            super(BREthereumLightNode.jniCreateLightNodeLES(client, network.getIdentifier(), paperKey),
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
        public JSON_RPC(ClientJSON_RPC client, BREthereumNetwork network, String paperKey) {
            super(BREthereumLightNode.jniCreateLightNodeJSON_RPC(client, network.getIdentifier(), paperKey),
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

        public void announceGasEstimate (int tid, String gasEstimate, int rid) {
            jniAnnounceGasEstimate(tid, gasEstimate, rid);
        }

        public void announceSubmitTransaction (int tid, String hash, int rid) {
            jniAnnounceSubmitTransaction(tid, hash, rid);
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

    public byte[] getAddressPublicKey () {
        return account.getPrimaryAddressPublicKey();
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
    // Callback Announcements
    //
    //
    // JNI Interface
    //

    protected static native long jniCreateLightNodeLES(Client client, long network, String paperKey);
    protected static native long jniCreateLightNodeLES_PublicKey(Client client, long network, byte[] publicKey);

    protected static native long jniCreateLightNodeJSON_RPC(Client client, long network, String paperKey);
    protected static native long jniCreateLightNodeJSON_RPC_PublicKey(Client client, long network, byte[] publicKey);

    protected native long jniLightNodeGetAccount();
    protected native String jniGetAccountPrimaryAddress(long accountId);
    protected native byte[] jniGetAccountPrimaryAddressPublicKey(long accountId);

    protected native long jniLightNodeGetWallet();
    protected native long jniLightNodeGetWalletToken (long tokenId);
    protected native long jniLightNodeCreateWalletToken(long tokenId);

    protected native String jniGetWalletBalance (long walletId, long unit);
    protected native void jniEstimateWalletGasPrice (long walletId);

    protected native void jniForceWalletBalanceUpdate(long wallet);

    //
    // Announcements
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

    protected native void jniAnnounceBalance (int wid, String balance, int rid);
    protected native void jniAnnounceGasPrice (int wid, String gasPrice, int rid);
    protected native void jniAnnounceGasEstimate (int tid, String gasEstimate, int rid);
    protected native void jniAnnounceSubmitTransaction (int tid, String hash, int rid);

    //
    // Wallet Transactions
    //
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

    protected native void jniTransactionEstimateGas(long walletId,
                                                    long transactionId);


    //
    // Transactions
    //
    protected native boolean jniTransactionHasToken (long transactionId);

    protected native String jniTransactionGetAmount(long transactionId, long unit);
    protected native String jniTransactionSourceAddress (long transactionId);
    protected native String jniTransactionTargetAddress (long transactionId);
    protected native String jniTransactionGetHash (long transactionId);
    protected native String jniTransactionGetGasPrice (long tranactionId, long unit);
    protected native long jniTransactionGetGasLimit (long tranactionId);
    protected native long jniTransactionGetGasUsed (long tranactionId);
    protected native long jniTransactionGetNonce (long tranactionId);
    protected native long jniTransactionGetBlockNumber (long tranactionId);
    protected native long jniTransactionGetBlockTimestamp (long tranactionId);
    protected native boolean jniTransactionIsConfirmed (long transactionId);

    //
    // Connect // Disconnect
    //
    protected native boolean jniLightNodeConnect ();
    protected native boolean jniLightNodeDisconnect ();

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

