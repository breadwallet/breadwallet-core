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

    }

    public interface ClientLES extends Client {

    }

    public interface ClientJSON_RPC extends Client {
        // typedef const char* (*JsonRpcGetBalance) (JsonRpcContext context, int id, const char *account);
        String getBalance (int id, String account);

        // typedef const char* (*JsonRpcGetGasPrice) (JsonRpcContext context, int id);
        String getGasPrice (int id);

        // typedef const char* (*JsonRpcEstimateGas) (JsonRpcContext context, int id, const char *to, const char *amount, const char *data);
        String getGasEstimate (int id, String to, String amount, String data);

        // typedef const char* (*JsonRpcSubmitTransaction) (JsonRpcContext context, int id, const char *transaction);
        String submitTransaction (int id, String rawTransaction);

        // typedef void (*JsonRpcGetTransactions) (JsonRpcContext context, int id, const char *account,
        //                                         JsonRpcAnnounceTransaction announceTransaction,
        //                                         JsonRpcAnnounceTransactionContext announceTransactionContext);
        void getTransactions (int id, String account);
    }

    //
    // Reference
    //
    static class Reference {
        WeakReference<BREthereumLightNode> node;
        long identifier;

        public Reference(BREthereumLightNode node, long identifier) {
            this.node = new WeakReference<>(node);
            this.identifier = identifier;
        }
    }

    //
    // Light Node Types
    //
    static public class LES extends BREthereumLightNode {
        protected LES(ClientLES client, BREthereumNetwork network) {
            super (BREthereumLightNode.jniCreateEthereumLightNodeLES(client, network.getIdentifier()),
                    client,
                    network);
        }
    }

    static public class JSON_RPC extends BREthereumLightNode {
        protected JSON_RPC(ClientJSON_RPC client, BREthereumNetwork network) {
            super (BREthereumLightNode.jniCreateEthereumLightNodeJSON_RPC(client, network.getIdentifier()),
                    client,
                    network);
        }
    }

     //
    // Light Node
    //
    WeakReference<Client> client;
    BREthereumNetwork network;

    public static BREthereumLightNode create (Client client, BREthereumNetwork network) {
        if (client instanceof ClientJSON_RPC)
            return new JSON_RPC((ClientJSON_RPC) client, network);
        else if (client instanceof ClientLES)
            return new LES ((ClientLES) client, network);
        else
            return null;
    }


    protected BREthereumLightNode(long jniReferenceAddress, Client client, BREthereumNetwork network) {
        super (jniReferenceAddress);
        this.client = new WeakReference<Client>(client);
        this.network = network;
    }

    //
    // Account
    //
    public BREthereumAccount createAccount(String paperKey) {
        return new BREthereumAccount(this, jniCreateEthereumLightNodeAccount(paperKey));
    }

    //
    // Wallet
    //
    public BREthereumWallet createWallet(BREthereumAccount account,
                                         BREthereumNetwork network) {
        return new BREthereumWallet(this,
                jniCreateEthereumLightNodeWallet(account.identifier, network.getIdentifier()),
                account,
                network);
    }

    public BREthereumWallet createWallet(BREthereumAccount account,
                                         BREthereumNetwork network,
                                         BREthereumToken token) {
        return new BREthereumWallet(this,
                jniCreateEthereumLightNodeWallet(account.identifier, network.getIdentifier()),
                account,
                network);
    }

    public void forceBalanceUpdate (BREthereumWallet wallet) {
        wallet.setBalance(jniForceWalletBalanceUpdate(wallet.identifier));
    }

    //
    // Transaction
    //


    //
    // JNI Interface
    //

    protected static native long jniCreateEthereumLightNodeLES (Client client, long network);

    protected static native long jniCreateEthereumLightNodeJSON_RPC (Client client, long network);

    protected native long jniCreateEthereumLightNodeAccount(String paperKey);

    protected native long jniCreateEthereumLightNodeWallet (long accountId, long networkId);

    protected native String jniGetAccountPrimaryAddress(long account);

    // TODO: Error
    protected native String jniForceWalletBalanceUpdate (long wallet);


    // TODO: Not going to work...
    public native void jniAnnounceTransaction(String from,
                                              String to,
                                              String amount,
                                              String gasLimit,
                                              String gasPrice,
                                              String data,
                                              String nonce);
    //    public native void disposeNative ();

    protected static native void initializeNative();

    static {
        initializeNative();
    }

 }

