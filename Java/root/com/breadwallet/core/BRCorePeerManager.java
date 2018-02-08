/*
 * BreadWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
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
package com.breadwallet.core;

import java.lang.ref.WeakReference;

/**
 *
 */
public class BRCorePeerManager extends BRCoreJniReference {

    //
    // Callback interface from Core, via JNI.
    //
    interface Listener {
        // func syncStarted()
        void syncStarted();

        // func syncStopped(_ error: BRPeerManagerError?)
        void syncStopped(String error);

        // func txStatusUpdate()
        void txStatusUpdate();

        // func saveBlocks(_ replace: Bool, _ blocks: [BRBlockRef?])
        void saveBlocks(boolean replace, BRCoreMerkleBlock[] blocks);

        // func savePeers(_ replace: Bool, _ peers: [BRPeer])
        void savePeers(boolean replace, BRCorePeer[] peers);

        // func networkIsReachable() -> Bool}
        boolean networkIsReachable();

        // Called on publishTransaction
        void txPublished (String error);
    }

    //
    // The Wallet
    //
    protected BRCoreWallet wallet;

    //
    // Weakly held (to avoid a likely self-reference) for use by JNI function
    // publishTransactionWithListener().
    //
    protected WeakReference<Listener> listener;


    public BRCorePeerManager(BRCoreChainParams params,
                             BRCoreWallet wallet,
                             double earliestKeyTime,
                             BRCoreMerkleBlock[] blocks,
                             BRCorePeer[] peers,
                             Listener listener) {
        // double time to int time.
        super(createCorePeerManager(params, wallet, earliestKeyTime, blocks, peers));
        installListener(listener);
        assert (null != this.listener);
        this.wallet = wallet;
    }

    /**
     *
     */
    public BRCorePeer.ConnectStatus getConnectStatus () {
        return BRCorePeer.ConnectStatus.fromValue(getConnectStatusValue());
    }

    private native int getConnectStatusValue ();

    /**
     * Connect to bitcoin peer-to-peer network (also call this whenever networkIsReachable()
     * status changes)
     */
    public native void connect();

    /*
    // connect to bitcoin peer-to-peer network (also call this whenever networkIsReachable() status changes)
    func connect() {
        if let fixedAddress = UserDefaults.customNodeIP {
            setFixedPeer(address: fixedAddress, port: UserDefaults.customNodePort ?? C.standardPort)
        }
        BRPeerManagerConnect(cPtr)
    }
*/

    //

    /**
     * Disconnect from bitcoin peer-to-peer network (may cause syncFailed(), saveBlocks() or
     * savePeers() callbacks to fire)
     */
    public native void disconnect();

    /**
     *
     */
    public native void rescan ();

    /**
     *
     * @return
     */
    public native long getEstimatedBlockHeight ();

    /**
     *
     * @return
     */
    public native long getLastBlockHeight ();

    /**
     *
     * @return
     */
    public native long getLastBlockTimestamp ();

    /**
     *
     * @param startHeight
     * @return
     */
    public native double getSyncProgress (long startHeight);

    /**
     * @return
     */
    public native int getPeerCount();

    /**
     *
     * @return
     */
    public native String getDownloadPeerName ();

    /**
     *
     * @param transaction
     */
    public void publishTransaction (BRCoreTransaction transaction) {
        publishTransactionWithListener(transaction, listener.get());

    }

    /**
     * A native method that will callback to BRCorePeerManager.Listener::txPublished.  We must
     * pass in the Listener, so that the Core function BRPeerManagerPublishTx() will know where
     * to callback into Java
     *
     * @param transaction
     * @param listener
     */
    protected native void publishTransactionWithListener (BRCoreTransaction transaction,
                                                          Listener listener);


    /**
     * @param txHash
     * @return
     */
    public native long getRelayCount (byte[] txHash);

    //
    // Constructor
    //

    private static native long createCorePeerManager(BRCoreChainParams params,
                                                     BRCoreWallet wallet,
                                                     double earliestKeyTime, // int
                                                     BRCoreMerkleBlock[] blocks,
                                                     BRCorePeer[] peers);

    protected native void installListener (BRCorePeerManager.Listener listener);

    //
    // Finalization
    //

    public native void disposeNative();

    protected static native void initializeNative ();

    static { initializeNative(); }

    //
    // toString()
    //
    @Override
    public String toString() {
        return "BRCorePeerManager {@ " + jniReferenceAddress +
                "\n  wallet addr: " + wallet.getReceiveAddress().stringify() +
                "\n  peer count : " + getPeerCount() +
                "\n  status     : " + getConnectStatus().name() +
                '}';
    }
}
