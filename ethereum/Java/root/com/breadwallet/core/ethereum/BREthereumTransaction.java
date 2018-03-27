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

import android.support.test.espresso.proto.action.ViewActions;

import com.breadwallet.tools.exceptions.PaymentRequestExpiredException;

import java.util.EnumMap;


/**
 *
 */
public class BREthereumTransaction extends BREthereumLightNode.ReferenceWithDefaultUnit {

    public enum Property {
        TARGET_ADDRESS(0),  // Ox'toAddr'
        SOURCE_ADDRESS(1),  // Ox'fromAddr'
        AMOUNT(2),          // ETH in WEI or TOKEN in INTEGER
        GAS_PRICE(3),       // ETH/Gas in WEI
        GAS_LIMIT(4),       // Gas
        GAS_USED(5),        // Gas
        NONCE(6),           // integer
        HASH(7),            // 0x'hash'
        BLOCK_NUMBER(8),    // integer
        BLOCK_TIMESTAMP(9); // seconds since EPOCH

        long jniValue;

        Property(long jniValue) {
            this.jniValue = jniValue;
        }

        static Property lookup (long jnivalue) {
            return values()[(int) jnivalue];
        }

    }

    /**
     * This caching is going to be a problem, a big problem, when for example, a transaction
     * gets blocked but properties has cached 'not blocked' values.
     */
    protected EnumMap<Property, String> properties;

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

    public String getProperty (Property property) {
        if (null == properties) installProperties();
        return properties.get(property);
    }

    //
    // Amount (handling units)
    //

    public String getAmount () {
        return getAmount(defaultUnit);
    }

    public String getAmount(BREthereumAmount.Unit unit) {
        validUnitOrException(unit);
        return node.get().jniGetTransactionAmount(identifier, unit.jniValue);
    }

    //
    // Support
    //
    static long[] jniValues = {
            Property.TARGET_ADDRESS.jniValue,
            Property.SOURCE_ADDRESS.jniValue,
            Property.AMOUNT.jniValue,
            Property.GAS_PRICE.jniValue,
            Property.GAS_LIMIT.jniValue,
            Property.GAS_USED.jniValue,
            Property.NONCE.jniValue,
            Property.HASH.jniValue,
            Property.BLOCK_NUMBER.jniValue,
            Property.BLOCK_TIMESTAMP.jniValue
    };

    protected void installProperties() {
        final String values[] = node.get().jniGetTransactionProperties(identifier, jniValues);
        this.properties = new EnumMap<Property, String>(Property.class){{
            for (long jniValue : jniValues)
                put (Property.lookup(jniValue), values[(int) jniValue]);
        }};
    }
}
