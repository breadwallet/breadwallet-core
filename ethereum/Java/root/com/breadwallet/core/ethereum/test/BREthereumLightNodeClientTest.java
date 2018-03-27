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

import static com.breadwallet.core.ethereum.BREthereumAmount.Unit.ETHER_GWEI;

/**
 *
 */
public class BREthereumLightNodeClientTest implements BREthereumLightNode.ClientJSON_RPC {
    static {
        if (System.getProperties().containsKey("light.node.test"))
            System.loadLibrary("Core");
    }

    private static final String RANDOM_TEST_PAPER_KEY =
            "axis husband project any sea patch drip tip spirit tide bring belt";

    private static final String USABLE_PAPER_KEY =
            "ginger settle marine tissue robot crane night number ramp coast roast critic";

    protected BREthereumLightNode node;

    public BREthereumLightNodeClientTest() {
    }

    @Override
    public void assignNode(BREthereumLightNode node) {
        this.node = node;
    }

    @Override
    public String getBalance(int id, String account) {
        asserting (null != node);
        return "0x123f";  // NON-NLS
    }

    @Override
    public String getGasPrice(int id) {
        asserting (null != node);
        return "0xffc0"; // NON-NLS
    }

    @Override
    public String getGasEstimate(int id, String to, String amount, String data) {
        asserting (null != node);
        return "0x77"; // NON-NLS
    }

    @Override
    public String submitTransaction(int id, String rawTransaction) {
        asserting (null != node);
        return "0x123abc456def"; // NON-NLS
    }

    @Override
    public void getTransactions(int id, String account) {
        asserting (null != node);
        // Query, then for-each...
        node.announceTransaction(
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

    protected void runTest () {
        // Create the node; reference through this.node
        new BREthereumLightNode.JSON_RPC(this, BREthereumNetwork.testnet, USABLE_PAPER_KEY);

        //
        // Test body
        //

        BREthereumWallet walletEther = node.getWallet();
        walletEther.setDefaultUnit(BREthereumAmount.Unit.ETHER_WEI);

        BREthereumWallet walletToken = node.createWallet(BREthereumToken.tokenBRD);
        walletToken.setDefaultUnit(BREthereumAmount.Unit.TOKEN_DECIMAL);

        asserting ("0".equals(walletEther.getBalance()));

        node.connect();
        walletEther.updateBalance();  // actually, node.connect() does this too.
        asserting (Integer.parseInt("123f", 16) == Integer.parseInt(walletEther.getBalance()));

        BREthereumTransaction trans1 = walletEther.createTransaction(
                "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                "11113000000000",
                BREthereumAmount.Unit.ETHER_WEI);
        walletEther.sign(trans1, USABLE_PAPER_KEY);
        walletEther.submit(trans1);

        asserting ("11113000000000".equals(trans1.getAmount()));
        asserting ("11113.000000000".equals(trans1.getAmount(ETHER_GWEI)));

        System.out.println ("==== Ether ====\n");
        for (BREthereumTransaction transaction : walletEther.getTransactions()) {
            System.out.println ("Transaction:" +
                    "\n    from: " + transaction.getProperty(BREthereumTransaction.Property.SOURCE_ADDRESS) +
                    "\n      to: " + transaction.getProperty(BREthereumTransaction.Property.TARGET_ADDRESS) +
                    "\n   nonce: " + transaction.getProperty(BREthereumTransaction.Property.NONCE) +
                    "\n\n");
        }

        System.out.println ("==== Token ====\n");
        for (BREthereumTransaction transaction : walletToken.getTransactions()) {
            System.out.println ("Transaction:" +
                    "\n    from: " + transaction.getProperty(BREthereumTransaction.Property.SOURCE_ADDRESS) +
                    "\n      to: " + transaction.getProperty(BREthereumTransaction.Property.TARGET_ADDRESS) +
                    "\n   nonce: " + transaction.getProperty(BREthereumTransaction.Property.NONCE) +
                    "\n\n");
        }

        walletEther.estimateGasPrice();
        walletEther.estimateGas(trans1);

        // Check trans1

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
