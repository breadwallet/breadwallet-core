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
import com.breadwallet.core.ethereum.BREthereumWallet;

/**
 *
 */
public class BREthereumTest implements BREthereumLightNode.ClientJSON_RPC {
    static {
        if (System.getProperties().containsKey("light.node.test"))
            System.loadLibrary("Core");
    }

    protected BREthereumLightNode node;

    public BREthereumTest(BREthereumNetwork network) {
        this.node = BREthereumLightNode.create(this, network);
    }

    public BREthereumLightNode getNode() {
        return node;
    }

    @Override
    public String getBalance(int id, String account) {
        return "0x123f";  // NON-NLS
    }

    @Override
    public String getGasPrice(int id) {
        return "0xffc0"; // NON-NLS
    }

    @Override
    public String getGasEstimate(int id, String to, String amount, String data) {
        return "0x77"; // NON-NLS
    }

    @Override
    public String submitTransaction(int id, String rawTransaction) {
        return "0x123abc456def"; // NON-NLS
    }

    @Override
    public void getTransactions(int id, String account) {
        // Query, then for-each...
        // TODO: In this form, not going to work.
        node.announceTransaction(
                "0x0ea166deef4d04aaefd0697982e6f7aa325ab69c",       // NON-NLS
                "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                "",// NON-NLS
                "11113000000000",
                "21000",
                "21000000000",
                "",
                "118");
    }

    private static final String RANDOM_TEST_PAPER_KEY =
            "axis husband project any sea patch drip tip spirit tide bring belt";

    private static final String USABLE_PAPER_KEY =
            "ginger settle marine tissue robot crane night number ramp coast roast critic";


    public static void main(String[] args) {
        BREthereumNetwork network = BREthereumNetwork.testnet;
        BREthereumTest test = new BREthereumTest(network);

        BREthereumLightNode node = test.getNode();
        BREthereumAccount account = node.createAccount(USABLE_PAPER_KEY);
        BREthereumWallet walletEther = node.createWallet(account, network);
        BREthereumWallet walletToken = node.createWallet(account, network, BREthereumToken.tokenBRD);

        asserting ("0.0".equals(walletEther.getBalance()));
        node.forceBalanceUpdate(walletEther);
        asserting (Integer.parseInt("123f", 16) == Integer.parseInt(walletEther.getBalance()));

        node.forceWalletTransactionUpdate(walletEther);

        System.out.println("No actual tests");
    }

    private static void asserting (boolean assertion) {
        if (!assertion) {
            throw new AssertionError();
        }
    }
}

