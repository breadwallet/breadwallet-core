/*
 * EthereumTest
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
package com.breadwallet.core.ethereum.test;


import com.breadwallet.core.ethereum.BREthereumAccount;
import com.breadwallet.core.ethereum.BREthereumAmount;
import com.breadwallet.core.ethereum.BREthereumLightNode;
import com.breadwallet.core.ethereum.BREthereumNetwork;
import com.breadwallet.core.ethereum.BREthereumToken;
import com.breadwallet.core.ethereum.BREthereumTransaction;
import com.breadwallet.core.ethereum.BREthereumWallet;

import static com.breadwallet.core.ethereum.BREthereumAmount.Unit.ETHER_ETHER;
import static com.breadwallet.core.ethereum.BREthereumAmount.Unit.ETHER_GWEI;

/**
 *
 */
public class BREthereumLightNodeClientTest implements
        BREthereumLightNode.ClientJSON_RPC,
        BREthereumLightNode.Listener {
    static {
        if (System.getProperties().containsKey("light.node.test"))
            System.loadLibrary("Core");
    }

    private static final String RANDOM_TEST_PAPER_KEY =
            "axis husband project any sea patch drip tip spirit tide bring belt";

    private static final String USABLE_PAPER_KEY =
            "ginger settle marine tissue robot crane night number ramp coast roast critic";

    protected BREthereumLightNode.JSON_RPC node;

    public BREthereumLightNodeClientTest() {
    }

    @Override
    public void assignNode(BREthereumLightNode node) {
        asserting (node instanceof BREthereumLightNode.JSON_RPC);
            this.node = (BREthereumLightNode.JSON_RPC) node;
    }

    @Override
    public void getBalance(int wid, String address, int rid) {
        node.announceBalance(wid, "0x123f", rid);
    }

    @Override
    public void getGasPrice(int wid, int rid) {
        node.announceGasPrice(wid, "0xffc0", rid); // NON-NLS
    }

    @Override
    public void getGasEstimate(int wid, int tid, String to, String amount, String data, int rid) {
        node.announceGasEstimate(tid, "0x77", rid); // NON-NLS
    }

    @Override
    public void submitTransaction(int wid, int tid, String rawTransaction, int rid) {
        node.announceSubmitTransaction(tid, "0x123abc456def", rid); // NON-NLS

    }

    @Override
    public void getTransactions(String address, int rid) {
        // Query, then for-each...
        node.announceTransaction(
                rid,
                "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                node.getAddress(),
                "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",        // NON-NLS
                "",  // NON-NLS
                "11113000000000",
                "21000",
                "21000000000",
                "0x",
                "118",
                "21000",
                "1627184",
                "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                "339050",
                "3",
                "1516477482",
                "0");

        // From 0: Swap to-from; later block number.  (Fake hash)
        node.announceTransaction(
                rid,
                "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d221",
                "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",        // NON-NLS
                node.getAddress(),
                "",  // NON-NLS
                "11113000000000",
                "21000",
                "21000000000",
                "0x",
                "118",
                "21000",
                "2627184",
                "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036bf",
                "339050",
                "3",
                "1516477482",
                "0");

        // From 0: earlier transactionIndex; lower nonce (Fake hash)
        node.announceTransaction(
                rid,
                "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d222",
                node.getAddress(),
                "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",        // NON-NLS
                "",  // NON-NLS
                "11113000000000",
                "21000",
                "21000000000",
                "0x",
                "117",
                "21000",
                "1627184",
                "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                "339050",
                "1",
                "1516477482",
                "0");

        // From 0: 'to' as tokenBread; data; block number before 'mod 2'.
        node.announceTransaction(
                rid,
                "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d223",
                node.getAddress(),
                "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",        // NON-NLS
                "",  // NON-NLS
                "11113000000000",
                "21000",
                "21000000000",
                "0xa9059cbb0000000000000000000000006c0fe9f8f018e68e2f0bee94ab41b75e71df094d00000000000000000000000000000000000000000000000000000000000003e8",
                "120",
                "21000",
                "1627184",
                "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                "339050",
                "0",
                "1516477482",
                "0");

        // From 0: identical
        node.announceTransaction(
                rid,
                "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                node.getAddress(),
                "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",        // NON-NLS
                "",  // NON-NLS
                "11113000000000",
                "21000",
                "21000000000",
                "0x",
                "118",
                "21000",
                "1627184",
                "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                "339050",
                "3",
                "1516477482",
                "0");

    }

    //
    // Listener
    //


    @Override
    public void handleWalletEvent(BREthereumWallet wallet, WalletEvent event) {
        System.err.println ("WalletEvent: " + event);
    }

    @Override
    public void handleTransactionEvent(BREthereumTransaction transaction, TransactionEvent event) {

    }

    protected void runTest () {
        // Create the node; reference through this.node
        new BREthereumLightNode.JSON_RPC(this, BREthereumNetwork.testnet, USABLE_PAPER_KEY);
        node.addListener(this);


        //
        // Test body
        //

        BREthereumWallet walletEther = node.getWallet();
        walletEther.setDefaultUnit(BREthereumAmount.Unit.ETHER_WEI);

        BREthereumWallet walletToken = node.createWallet(BREthereumToken.tokenBRD);
        walletToken.setDefaultUnit(BREthereumAmount.Unit.TOKEN_DECIMAL);

        asserting ("0".equals(walletEther.getBalance()));

        System.out.println ("Connect");
        node.connect();

        BREthereumTransaction trans1 = walletEther.createTransaction(
                "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                "11113000000000",
                BREthereumAmount.Unit.ETHER_WEI);
        walletEther.sign(trans1, USABLE_PAPER_KEY);
        walletEther.submit(trans1);

        asserting ("11113000000000".equals(trans1.getAmount()));
        asserting ("11113.000000000".equals(trans1.getAmount(ETHER_GWEI)));

        asserting (21000 == trans1.getGasLimit());
        asserting ("40.000000000".equals(trans1.getGasPrice(ETHER_GWEI)));
        asserting ("840000.000000000".equals(trans1.getFee(ETHER_GWEI)));

        // Fee for 1 WEI is 840000 GWEI
        asserting ("840000000000000".equals(walletEther.transactionEstimatedFee("1")));
        asserting ("840000.000000000".equals(walletEther.transactionEstimatedFee("1", ETHER_ETHER, ETHER_GWEI)));

        System.out.println ("==== Ether ====\n");
        for (BREthereumTransaction transaction : walletEther.getTransactions()) {
            System.out.println ("Transaction:" +
                    "\n        from: " + transaction.getSourceAddress() +
                    "\n          to: " + transaction.getTargetAddress() +
                    "\n    gasPrice: " + transaction.getGasPrice(BREthereumAmount.Unit.ETHER_WEI) +
                    "\n    gasLimit: " + transaction.getGasLimit() +
                    "\n     gasUsed: " + transaction.getGasUsed() +
                    "\n       nonce: " + transaction.getNonce() +
                    "\n   blkNumber: " + transaction.getBlockNumber() +
                    "\n     blkTime: " + transaction.getBlockTimestamp() +
                    "\n\n");
        }

        System.out.println ("==== Token ====\n");
        for (BREthereumTransaction transaction : walletToken.getTransactions()) {
            System.out.println ("Transaction:" +
                    "\n    from: " + transaction.getSourceAddress() +
                    "\n      to: " + transaction.getTargetAddress() +
                    "\n   nonce: " + transaction.getNonce() +
                    "\n\n");
        }

        // Wait until balance updates.
        asserting (Integer.parseInt("123f", 16) == Integer.parseInt(walletEther.getBalance()));
        walletEther.estimateGasPrice();
        walletEther.estimateGas(trans1);

        // Check trans1


        byte[] publicKey = node.getAddressPublicKey();
        asserting (65 == publicKey.length);

        BREthereumLightNode node1 = this.node;
        BREthereumLightNode node2 = new BREthereumLightNode.JSON_RPC(this, BREthereumNetwork.testnet, publicKey);
        asserting (node1.getAddress().equals(node2.getAddress()));

        System.out.println ("Disconnect");
        node.disconnect();
    }

    private static boolean runMain = true;

    public static void main(String[] args) {
        if (!runMain)return;
        runMain = false;

        new BREthereumLightNodeClientTest()
                .runTest();

        System.out.println("Success");
    }

    private static void asserting (boolean assertion) {
        if (!assertion) {
            throw new AssertionError();
        }
    }
}
