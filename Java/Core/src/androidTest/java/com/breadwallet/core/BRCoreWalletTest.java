package com.breadwallet.core;

import org.junit.Test;

import static org.junit.Assert.*;

public class BRCoreWalletTest {
    static { System.loadLibrary("core"); }

    @Test
    public void runTests() {

        BRCoreMasterPubKey pubKey = new BRCoreMasterPubKey("a random seed".getBytes(), true);
        try {
            BRCoreWallet wallet = new BRCoreWallet("/sdcard/coreTransactions", pubKey, null);
            BRCorePeerManager manager = new BRCorePeerManager(BRCoreChainParams.testnetChainParams,
                     wallet,
                    0,
                    "/sdcard/coreBlocks",
                    "/sdcard/corePeers",
                    null

                    );
        } catch (BRCoreWallet.WalletExecption e) {

        }
    }


}