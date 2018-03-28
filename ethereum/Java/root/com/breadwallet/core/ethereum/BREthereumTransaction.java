/*
 * EthereumTransaction
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
 *
 */
public class BREthereumTransaction extends BREthereumLightNode.ReferenceWithDefaultUnit {

    /**
     *
     * @param node
     * @param identifier
     * @param unit  The transaction's unit; should be identical with that unit used to create
     *              the transaction identifier.
     */
    protected BREthereumTransaction (BREthereumLightNode node, long identifier, BREthereumAmount.Unit unit) {
        super(node, identifier, unit);
    }

    public boolean isConfirmed () {
        return node.get().jniTransactionIsConfirmed(identifier);
    }

    public String getSourceAddress () {
        return node.get().jniTransactionSourceAddress(identifier);
    }

    public String getTargetAddress () {
        return node.get().jniTransactionTargetAddress(identifier);
    }

    //
    // Amount (handling units)
    //

    public String getAmount () {
        return getAmount(defaultUnit);
    }

    public String getAmount(BREthereumAmount.Unit unit) {
        validUnitOrException(unit);
        return node.get().jniTransactionGetAmount(identifier, unit.jniValue);
    }

    //
    // Gas Price, Limit, Used
    //
    public String getGasPrice (BREthereumAmount.Unit unit) {
        assert (!unit.isTokenUnit());
        return node.get().jniTransactionGetGasPrice(identifier, unit.jniValue);
    }

    public long getGasLimit () {
        return node.get().jniTransactionGetGasLimit(identifier);
    }

    public long getGasUsed () {
        return node.get().jniTransactionGetGasUsed(identifier);
    }

    //
    // Nonce
    //
    public long getNonce () {
        return node.get().jniTransactionGetNonce(identifier);
    }

    //
    // Block Number, Timestamp
    //
    public long getBlockNumber () {
        return node.get().jniTransactionGetBlockNumber(identifier);
    }

    public long getBlockTimestamp () {
        return node.get().jniTransactionGetBlockTimestamp(identifier);
    }
}
