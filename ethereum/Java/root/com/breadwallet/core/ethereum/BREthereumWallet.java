/*
 * EthereumWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 3/20/18.
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

import com.breadwallet.core.ethereum.BREthereumAmount.Unit;

/**
 * An EthereumWallet holds either ETHER or TOKEN values.
 */
public class BREthereumWallet extends BREthereumLightNode.ReferenceWithDefaultUnit {

    private BREthereumAccount account;
    private BREthereumNetwork network;
    private BREthereumToken   token = null;

    public BREthereumAccount getAccount () {
        return account;
    }

    public BREthereumNetwork getNetwork () {
        return network;
    }

    public BREthereumToken getToken () {
        return token;
    }

    public boolean walletHoldsEther () {
        return null == token;
    }

    //
    // Constructors
    //

    protected BREthereumWallet (BREthereumLightNode node, long identifier,
                                BREthereumAccount account,
                                BREthereumNetwork network) {
        super (node, identifier, Unit.ETHER_ETHER);
        this.account = account;
        this.network = network;
    }

    protected BREthereumWallet (BREthereumLightNode node, long identifier,
                                BREthereumAccount account,
                                BREthereumNetwork network,
                                BREthereumToken token) {
        this (node, identifier, account, network);
        this.token = token;
        this.defaultUnit = Unit.TOKEN_DECIMAL;
        this.defaultUnitUsesToken = true;
    }

    //
    // Balance
    //

    public String getBalance () {
        return getBalance(defaultUnit);
    }

    public String getBalance(Unit unit) {
        validUnitOrException(unit);
        return node.get().jniGetWalletBalance(identifier, unit.jniValue);
    }

    //
    // Transactions
    //
    public BREthereumTransaction createTransaction(String targetAddress,
                                                   String amount,
                                                   Unit amountUnit) {
        BREthereumLightNode lightNode = node.get();

        // We'll assume that the transaction's default unit is the same unit as provided
        // for `amount`.  Should not be a bad assumption.  However, arguably we could set
        // the transaction's unit as the wallet's unit.
        return new BREthereumTransaction(lightNode,
                lightNode.jniCreateTransaction(identifier,
                        targetAddress,
                        amount,
                        amountUnit.jniValue),
                amountUnit);
    }

    // getAll
    // getByX
    // getByY

    // sign
    public void sign (BREthereumTransaction transaction,
                      String paperKey) {
        node.get().jniSignTransaction(identifier, transaction.identifier, paperKey);
    }

    // submit
    public void submit (BREthereumTransaction transaction) {
        node.get().jniSubmitTransaction(identifier, transaction.identifier);
    }
 }
