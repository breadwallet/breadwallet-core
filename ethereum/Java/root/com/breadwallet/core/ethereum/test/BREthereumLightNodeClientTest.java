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
import com.breadwallet.core.ethereum.BREthereumLightNode;
import com.breadwallet.core.ethereum.BREthereumNetwork;
import com.breadwallet.core.ethereum.BREthereumToken;
import com.breadwallet.core.ethereum.BREthereumTransaction;
import com.breadwallet.core.ethereum.BREthereumWallet;

/**
 *
 */
public class BREthereumLightNodeClientTest implements BREthereumLightNode.ClientJSON_RPC {
    static {
        if (System.getProperties().containsKey("light.node.test"))
            System.loadLibrary("Core");
    }

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
                "",
                "118",
                "21000",
                "1627184",
                "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                "339050",
                "3",
                "1516477482",
                "0");
    }

    private static final String RANDOM_TEST_PAPER_KEY =
            "axis husband project any sea patch drip tip spirit tide bring belt";

    private static final String USABLE_PAPER_KEY =
            "ginger settle marine tissue robot crane night number ramp coast roast critic";

    private static boolean runMain = true;
    private static int runCount = 0;

    public static void main(String[] args) {
        if (!runMain)return;
        runMain = false;
        runCount++;

        BREthereumLightNodeClientTest client = new BREthereumLightNodeClientTest();

        BREthereumLightNode node = new BREthereumLightNode.JSON_RPC(client, BREthereumNetwork.testnet, USABLE_PAPER_KEY);
        BREthereumAccount account = node.getAccount();

        BREthereumWallet walletEther = node.getWallet();
        walletEther.setDefaultUnit(BREthereumWallet.Unit.ETHER_WEI);

        BREthereumWallet walletToken = node.createWallet(BREthereumToken.tokenBRD);
        walletToken.setDefaultUnit(BREthereumWallet.Unit.TOKEN_DECIMAL);

        asserting ("0".equals(walletEther.getBalance()));

        node.connect();
        node.forceBalanceUpdate(walletEther);
        asserting (Integer.parseInt("123f", 16) == Integer.parseInt(walletEther.getBalance()));

        node.forceTransactionUpdate(walletEther);

        BREthereumTransaction trans1 = walletEther.createTransaction(
                "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                "11113000000000",
                BREthereumWallet.Unit.ETHER_WEI);
        walletEther.sign(trans1, USABLE_PAPER_KEY);
        walletEther.submit(trans1);

        node.disconnect();
        System.out.println("Success");
    }

    private static void asserting (boolean assertion) {
        if (!assertion) {
            throw new AssertionError();
        }
    }
}
