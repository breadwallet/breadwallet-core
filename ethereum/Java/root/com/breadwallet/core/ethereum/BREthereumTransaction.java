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

import java.util.EnumMap;


/**
 *
 */
public class BREthereumTransaction extends BREthereumLightNode.ReferenceWithDefaultUnit {

    public enum Property {
        TARGET_ADDRESS(0),  // toAddr
        SOURCE_ADDRESS(1),  // fromAddr
        // hash
        // timestamp
        // confirmation
        // gasLimit
        // gasUsed
        // gasPrice

        // ...
        NONCE(2);

        long jniValue;

        Property(long jniValue) {
            this.jniValue = jniValue;
        }

        static Property lookup (long jnivalue) {
            return values()[(int) jnivalue];
        }

        static long[] jniValues = {
            Property.TARGET_ADDRESS.jniValue,
            Property.SOURCE_ADDRESS.jniValue,
            Property.NONCE.jniValue
        };
    }

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
        installProperties();
    }

    public String getProperty (Property property) {
        return properties.get(property);
    }

    public String getSourceAddress () {
        return properties.get(Property.SOURCE_ADDRESS);
    }

    // ...

    //
    // Amount
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
    protected void installProperties() {
        final String values[] = node.get().jniGetTransactionProperties(identifier, Property.jniValues);
        this.properties = new EnumMap<Property, String>(Property.class){{
            for (long jniValue : Property.jniValues)
                put (Property.lookup(jniValue), values[(int) jniValue]);
        }};
    }
}
