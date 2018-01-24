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
package com.breadwallet.core.test;

import com.breadwallet.core.BRCoreChainParams;
import com.breadwallet.core.BRCoreMasterPubKey;
import com.breadwallet.core.BRCoreWalletManager;

import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;
import java.util.function.Consumer;

/**
 *
 */
public class BRWalletManager extends BRCoreWalletManager {
    static {
        if (System.getProperties().containsKey("wallet.test"))
            System.loadLibrary("Core");
    }

    public BRWalletManager(BRCoreMasterPubKey masterPubKey,
                           BRCoreChainParams chainParams,
                           double earliestPeerTime) {
        super(masterPubKey, chainParams, earliestPeerTime);
    }

//    @Override
//    protected BRCorePeer[] loadPeers() {
//        return new BRCorePeer[] {
//                new BRCorePeer(this.chainParams.getJniMagicNumber())
//        };
//    }

    @Override
    public void syncStarted() {
        System.err.println ("syncStarted (BRWalletManager)");
    }

    private static final double BIP39_CREATION_TIME= 1388534400.0;
    private static final String SOME_RANDOM_TEST_PAPER_KEY =
            "axis husband project any sea patch drip tip spirit tide bring belt";

    public static void main (String[] args) {

        Configuration configuration = parseArguments(
                null == args || 0 == args.length ? new String[] { "-test" } : args);

        final BRCoreMasterPubKey masterPubKey =
                new BRCoreMasterPubKey(SOME_RANDOM_TEST_PAPER_KEY.getBytes());

        final List<BRWalletManager> walletManagers = new LinkedList<>();

        for (BRCoreChainParams chainParams : configuration.listOfParams) {
            walletManagers.add(
                    new BRWalletManager(masterPubKey,
                            chainParams,
                            BIP39_CREATION_TIME));
        }

        if (!walletManagers.isEmpty())
            describeWalletManager(walletManagers.get(0));

        for (BRWalletManager walletManager : walletManagers) {
            walletManager.getPeerManager().connect();;
        }

        try {
            Thread.sleep(120 * 60 * 1000);
            System.err.println("Times Up - Done");

            Thread.sleep(2 * 1000);
        } catch (InterruptedException ex) {
            System.err.println("Interrupted - Done");
        }

        for (BRWalletManager walletManager : walletManagers) {
            walletManager.getPeerManager().disconnect();;
        }

        System.exit(0);

    }
// From test.c
//
//        ALL TESTS PASSED
//        wallet created with first receive address: 15RBcXQMTfebbAfUFeBbcDfs1fVvPayWdU
//                :::0 PeerManager: ChainParams: Port: 8333, MagicNumber: d9b4bef9
//                :::0 PeerManager: Wallet: Balance: 0, FeePerKB: 50000
//        sync started
//        201.210.38.158:8333 connecting
//        90.17.54.173:8333 connecting
//        185.35.138.84:8333 connecting
//        185.35.138.84:8333 socket connected
//        185.35.138.84:8333 sending version
//        185.35.138.84:8333 got version 70015, useragent:"/Satoshi:0.15.1/"
//        185.35.138.84:8333 sending verack
//        185.35.138.84:8333 got verack in 0.193193s
//        185.35.138.84:8333 handshake completed
//        185.35.138.84:8333 connected with lastblock: 505969
//        185.35.138.84:8333 sending filterload
//        185.35.138.84:8333 calling getheaders with 2 locators: [e2d0358276dcfdff72ce027d75b0b7d3b8c6ea3dc3e17ba70a00000000000000, 6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000]
//        185.35.138.84:8333 sending getheaders
//        185.35.138.84:8333 dropping sendheaders, length 0, not implemented
//        185.35.138.84:8333 got ping
//        185.35.138.84:8333 sending pong
//        185.35.138.84:8333 got feefilter with rate 1000
//        185.35.138.84:8333 got 2000 header(s)

    private static void describeWalletManager (BRWalletManager manager) {
        System.err.println("MasterPubKey: " + manager.masterPubKey.toString());
        System.err.println("\nChainParams: " + manager.chainParams.toString());
        System.err.println("\n" + manager.toString());

        // Wallet and PeerManager
        System.err.println("\n" + manager.getWallet());
        System.err.println("\n" + manager.getPeerManager());
    }

    private static Configuration parseArguments (String[] args) {
        List<BRCoreChainParams> listOfParams = new LinkedList<>();

        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "-main":
                    listOfParams.add(BRCoreChainParams.mainnetChainParams);
                    break;
                case "-test":
                    listOfParams.add(BRCoreChainParams.testnetChainParams);
                    break;
                case "-cash":
                    listOfParams.add(BRCoreChainParams.bcashChainParams);
                    break;
                default:
                    System.err.println ("Unexpected argument (" + args[i] + ") - ignoring");
                    break;
            }
        }
        return new Configuration(listOfParams);
    }

    private static class Configuration {
        List<BRCoreChainParams> listOfParams;

        public Configuration(List<BRCoreChainParams> listOfParams) {
            this.listOfParams = listOfParams;
        }
    }
}
