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
import java.util.concurrent.Executor;

/**
 *
 */
public class BRCorePeerManager extends BRCoreJniReference {

    //
    //
    //
    interface Listener {
        // func syncStarted()
        void syncStarted();

        // func syncStopped(_ error: BRPeerManagerError?)
        void syncStopped(int error);

        // func txStatusUpdate()
        void txStatusUpdate();

        // func saveBlocks(_ replace: Bool, _ blocks: [BRBlockRef?])
        void saveBlocks(boolean replace, BRCoreMerkleBlock[] blocks);

        // func savePeers(_ replace: Bool, _ peers: [BRPeer])
        void savePeers(boolean replace, BRCorePeer[] peers);

        // func networkIsReachable() -> Bool}
        boolean networkIsReachable();

        void txPublished (int error);

        BRCoreMerkleBlock createMerkleBlock (long jniReferenceAddress);
        BRCorePeer createPeer (long jniReferenceAddress);
    }

    //
    // A Listener that runs on a provided Executor.  This gets the Listener computation off of
    // the Core thread.
    //
    public static class WrappedExecutorListener implements Listener {
        Listener listener;
        Executor executor;

        public WrappedExecutorListener(Listener listener, Executor executor) {
            this.listener = listener;
            this.executor = executor;
        }

        @Override
        public void syncStarted() {
            executor.execute(new Runnable() {
                @Override
                public void run() {
                    listener.syncStarted();
                }
            });
        }

        @Override
        public void syncStopped(final int error) {
            executor.execute(new Runnable() {
                @Override
                public void run() {
                    listener.syncStopped(error);
                }
            });
        }

        @Override
        public void txStatusUpdate() {
            executor.execute(new Runnable() {
                @Override
                public void run() {
                    listener.txStatusUpdate();
                }
            });
        }

        @Override
        public void saveBlocks(final boolean replace, final BRCoreMerkleBlock[] blocks) {
            executor.execute(new Runnable() {
                @Override
                public void run() {
                    listener.saveBlocks(replace, blocks);
                }
            });
        }

        @Override
        public void savePeers(final boolean replace, final BRCorePeer[] peers) {
            executor.execute(new Runnable() {
                @Override
                public void run() {
                    listener.savePeers(replace, peers);
                }
            });
        }

        @Override
        public boolean networkIsReachable() {
            return listener.networkIsReachable();
        }

        @Override
        public void txPublished(final int error) {
            executor.execute(new Runnable() {
                @Override
                public void run() {
                    listener.txPublished(error);
                }
            });
        }

        @Override
        public BRCoreMerkleBlock createMerkleBlock(long jniReferenceAddress) {
            return listener.createMerkleBlock(jniReferenceAddress);
        }

        @Override
        public BRCorePeer createPeer(long jniReferenceAddress) {
            return listener.createPeer (jniReferenceAddress);
        }
    }

    //
    // The Wallet
    //
    protected BRCoreWallet wallet;

    //
    // Weakly held (to avoid a likely self-reference) for use by JNI functions
    // createCorePeerManager() and publishTransactionWithListener().
    //
    protected WeakReference<Listener> listener;


    public BRCorePeerManager(BRCoreChainParams params,
                             BRCoreWallet wallet,
                             double earliestKeyTime,
                             BRCoreMerkleBlock[] blocks,
                             BRCorePeer[] peers,
                             Listener listener) {
        // double time to int time.
        super(createCorePeerManager(params, wallet, earliestKeyTime, blocks, peers, listener));
        this.listener = new WeakReference<>(listener);
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
    public native void publishTransactionWithListener (BRCoreTransaction transaction,
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
                                                     BRCorePeer[] peers,
                                                     Listener listener);

    //
    // Finalization
    //

    public native void disposeNative();

    @Override
    public String toString() {
        return "BRCorePeerManager {@ " + jniReferenceAddress +
                "\n  wallet addr: " + wallet.getReceiveAddress().stringify() +
                "\n  peer count : " + getPeerCount() +
                "\n  status     : " + getConnectStatus().name() +
                '}';
    }
}
