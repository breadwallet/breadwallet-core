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

/**
 * An EthereumWallet holds either ETHER or TOKEN values.
 */
public class BREthereumWallet extends BREthereumLightNode.Reference {

    //
    // The Unit to use when displaying amounts - such as a wallet balance.
    //
    public enum Unit {
        TOKEN_DECIMAL(0),
        TOKEN_INTEGER(1),

        ETHER_WEI(0),
        ETHER_GWEI(3),
        ETHER_ETHER(6);

        // jniValue must match Core enum for:
        //    BREthereumUnit and BREthereumTokenQuantityUnit
        protected long jniValue;

        Unit(long jniValue) {
            this.jniValue = jniValue;
        }
    };

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
        super (node, identifier);
        this.account = account;
        this.network = network;
        this.defaultUnit = Unit.ETHER_ETHER;
    }

    protected BREthereumWallet (BREthereumLightNode node, long identifier,
                                BREthereumAccount account,
                                BREthereumNetwork network,
                                BREthereumToken token) {
        this (node, identifier, account, network);
        this.token = token;
        this.defaultUnit = Unit.TOKEN_INTEGER;
    }

    //
    // (Default) Unit
    //
    protected Unit defaultUnit;

    public Unit getDefaultUnit() {
        return defaultUnit;
    }

    public void setDefaultUnit(Unit unit) {
        validUnitOrException(unit);
        this.defaultUnit = unit;
    }

    protected boolean validUnit(Unit unit) {
        return (null == token
                ? (unit == Unit.ETHER_WEI || unit == Unit.ETHER_GWEI || unit == Unit.ETHER_ETHER)
                : (unit == Unit.TOKEN_DECIMAL || unit == Unit.TOKEN_INTEGER));
    }

    protected void validUnitOrException (Unit unit) {
        if (!validUnit(unit))
            throw new IllegalArgumentException("Invalid Unit for Wallet type: " + unit.toString());
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
    public BREthereumTransaction createTransaction (String targetAddress,
                                                    String amount,
                                                    Unit amountUnit) {
        long transaction = node.get().jniCreateTransaction(identifier, targetAddress, amount, amountUnit.jniValue);
        return new BREthereumTransaction(node.get(), transaction);
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
