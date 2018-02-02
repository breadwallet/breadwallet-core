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

/**
 * A BRCoreWalletManger instance manages a single wallet, and that wallet's individual connection
 * to the bitcoin network.  After instantiating a BRCoreWalletManager object, call
 * myWalletManager.peerManager.connect() to begin syncing.
 *
 */
public class BRCoreWalletManager implements
        BRCorePeerManager.Listener,
        BRCoreWallet.Listener {

    protected BRCoreMasterPubKey masterPubKey;

    protected BRCoreChainParams chainParams;

    double earliestPeerTime;

//    protected Optional<BRCoreWallet> wallet;
    BRCoreWallet wallet;

//    protected Optional<BRCorePeerManager> peerManager;
    BRCorePeerManager peerManager;
    
    //
    //
    //
    public BRCoreWalletManager(BRCoreMasterPubKey masterPubKey,
                               BRCoreChainParams chainParams,
                               double earliestPeerTime) {
        this.masterPubKey = masterPubKey;
        this.chainParams = chainParams;
        this.earliestPeerTime = earliestPeerTime;
    }

    //
    // Wallet
    //
    //    public Optional<BRCoreWallet> getWallet() {
    //        if (null == wallet) {
    //            wallet = Optional.of (createWallet());
    //        }
    //        return wallet;
    //    }
    public BRCoreWallet getWallet () {
        if (null == wallet) {
            wallet = createWallet();
        }
        return wallet;
    }

    /**
     * Factory method to create a BRCoreWallet (or subtype).
     *
     * @return The BRCoreWallet managed by this BRCoreWalletManager
     */

    protected BRCoreWallet createWallet () {
        return new BRCoreWallet (loadTransactions(), masterPubKey,
                new WrappedExceptionWalletListener (this));
    }

    //
    // Peer Manager
    //
    //    public Optional<BRCorePeerManager> getPeerManager() {
    //        if (null == peerManager) {
    //            peerManager = getWallet()
    //                    // .map (this::createPeerManager)
    //                    .map(new Function<BRCoreWallet, BRCorePeerManager>() {
    //                        @Override
    //                        public BRCorePeerManager apply(BRCoreWallet wallet) {
    //                            return createPeerManager(wallet);
    //                        }
    //                    });
    //        }
    //        return peerManager;
    //    }
    public BRCorePeerManager getPeerManager () {
        if (null == peerManager) {
            BRCoreWallet wallet = getWallet();
            if (null != wallet) {
                peerManager = createPeerManager(wallet);
            }
        }
        return peerManager;
    }

    /**
     * Factory method to create a BRCorePeerManager (or subtype).
     *
     * @param wallet The wallet
     * @return A BRCorePeerManager for the provided wallet.
     */
    protected BRCorePeerManager createPeerManager (BRCoreWallet wallet) {
        return new BRCorePeerManager(chainParams, wallet, earliestPeerTime, loadBlocks(), loadPeers(),
                new WrappedExceptionPeerManagerListener(this));
    }

    //
    // Support
    //

    protected BRCoreTransaction[] loadTransactions ()
    {
        return new BRCoreTransaction[0];
    }

    protected BRCoreMerkleBlock[] loadBlocks ()
    {
        return new BRCoreMerkleBlock[0];
    }

    protected BRCorePeer[] loadPeers ()
    {
        return new BRCorePeer[0];
    }

    //
    // BRCorePeerManager.Listener
    //

    @Override
    public void syncStarted() {
        System.err.println ("syncStarted");
    }

    @Override
    public void syncStopped(int error) {
        System.err.println ("syncStopped: " + error);
    }

    @Override
    public void txStatusUpdate() {
        System.err.println ("txStatusUpdate");
    }

    @Override
    public void saveBlocks(boolean replace, BRCoreMerkleBlock[] blocks) {
        // System.err.println ("saveBlocks: " + Integer.toBinaryString(blocks.length));
        System.err.println(String.format("saveBlocks: %d", blocks.length));
        for (int i = 0; i < blocks.length; i++)
            System.err.println(blocks[i].toString());
    }

    @Override
    public void savePeers(boolean replace, BRCorePeer[] peers) {
        System.err.println(String.format("savePeers: %d", peers.length));
        for (int i = 0; i < peers.length; i++)
            System.err.println(peers[i].toString());
    }

    @Override
    public boolean networkIsReachable() {
        // System.err.println ("networkIsReachable");
        return true;
    }

    @Override
    public void txPublished(int error) {
        System.err.println (String.format ("txPublished: %d", error));
    }

    @Override
    public BRCoreMerkleBlock createMerkleBlock(long jniReferenceAddress) {
        return new BRCoreMerkleBlock (jniReferenceAddress);
    }

    @Override
    public BRCorePeer createPeer(long jniReferenceAddress) {
        return new BRCorePeer (jniReferenceAddress);
    }

    //
    // BRCoreWallet.Listener
    //

    @Override
    public void balanceChanged(long balance) {
        System.err.println (String.format ("balanceChanged: %d", balance));
        System.err.println (wallet.toString());
    }

    @Override
    public void onTxAdded(BRCoreTransaction transaction) {
        System.err.println ("onTxAdded");
        System.err.println(transaction.toString());
        for (BRCoreTransactionInput input : transaction.getInputs())
            System.err.println (input.toString());
        for (BRCoreTransactionOutput output : transaction.getOutputs())
            System.err.println (output.toString());
    }

    @Override
    public void onTxUpdated(String hash, int blockHeight, int timeStamp) {
        System.err.println ("onTxUpdated");
    }

    @Override
    public void onTxDeleted(String hash, int notifyUser, int recommendRescan) {
        System.err.println ("onTxDeleted");
    }

    @Override
    public BRCoreTransaction createTransaction(long jniReferenceAddress) {
        return new BRCoreTransaction (jniReferenceAddress);
    }

    @Override
    public BRCoreAddress createAddress(long jniReferenceAddress) {
        return new BRCoreAddress (jniReferenceAddress);
    }

    //
    // Object methods
    //
    @Override
    public String toString() {
        return "BRCoreWalletManager{" +
                "\n  masterPubKey      : " + masterPubKey +
                "\n  chainParams       : " + chainParams +
                "\n  earliest peer time: " + earliestPeerTime +
                "\n  wallet rcv addr   : " + (wallet != null ? wallet.getReceiveAddress().stringify() : "") +
                "\n  peerManager status: " + (peerManager != null ? peerManager.getConnectStatus().name() : "") +
                '}';
    }

    // ============================================================================================
    //
    // Callbacks from JNI code that throw an exception are QUIETLY SWALLOWED.  We'll provide
    // a wrapper class, implementing each Listener used for callbacks.  The wrapper class
    // will catch any exception and issue some warning, or something.

    //
    // Exception Wrapped PeerManagerListener
    //
    static private class WrappedExceptionPeerManagerListener implements BRCorePeerManager.Listener {
        private BRCorePeerManager.Listener listener;

        public WrappedExceptionPeerManagerListener(BRCorePeerManager.Listener listener) {
            this.listener = listener;
        }

        //        private <T> void safeHandler (Supplier<Void> supplier) {
        //            try { supplier.get(); }
        //            catch (Exception ex) {
        //                ex.printStackTrace(System.err);
        //            }
        //        }

        @Override
        public void syncStarted() {
            try { listener.syncStarted(); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
            }
        }

        @Override
        public void syncStopped(int error) {
            try { listener.syncStopped(error); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
            }
        }

        @Override
        public void txStatusUpdate() {
            try { listener.txStatusUpdate(); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
            }
        }

        @Override
        public void saveBlocks(boolean replace, BRCoreMerkleBlock[] blocks) {
            try { listener.saveBlocks(replace, blocks); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
            }
        }

        @Override
        public void savePeers(boolean replace, BRCorePeer[] peers) {
            try { listener.savePeers(replace, peers); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
            }
        }

        @Override
        public boolean networkIsReachable() {
            try { return listener.networkIsReachable(); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
                return false;
            }
        }

        @Override
        public void txPublished(int error) {
            try { listener.txPublished(error); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
            }
        }

        @Override
        public BRCoreMerkleBlock createMerkleBlock(long jniReferenceAddress) {
            try { return listener.createMerkleBlock(jniReferenceAddress); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
                return null;
            }
        }

        @Override
        public BRCorePeer createPeer(long jniReferenceAddress) {
            try { return listener.createPeer(jniReferenceAddress); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
                return null;
            }
        }
    }

    //
    // Exception Wrapped WalletListener
    //
    static private class WrappedExceptionWalletListener implements BRCoreWallet.Listener {
        private BRCoreWallet.Listener listener;

        public WrappedExceptionWalletListener(BRCoreWallet.Listener listener) {
            this.listener = listener;
        }

        @Override
        public void balanceChanged(long balance) {
            try { listener.balanceChanged(balance); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
            }
        }

        @Override
        public void onTxAdded(BRCoreTransaction transaction) {
            try { listener.onTxAdded(transaction); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
            }
        }

        @Override
        public void onTxUpdated(String hash, int blockHeight, int timeStamp) {
            try { listener.onTxUpdated(hash, blockHeight, timeStamp); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
            }
        }

        @Override
        public void onTxDeleted(String hash, int notifyUser, int recommendRescan) {
            try { listener.onTxDeleted (hash, notifyUser, recommendRescan); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
            }
        }

        @Override
        public BRCoreTransaction createTransaction(long jniReferenceAddress) {
            try { return listener.createTransaction (jniReferenceAddress); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
                return null;
            }
        }

        @Override
        public BRCoreAddress createAddress(long jniReferenceAddress) {
            try { return listener.createAddress (jniReferenceAddress); }
            catch (Exception ex) {
                ex.printStackTrace(System.err);
                return null;
            }
        }
    }
}
