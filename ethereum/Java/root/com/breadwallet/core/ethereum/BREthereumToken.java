/*
 * EthereumToken
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


import com.breadwallet.core.BRCoreJniReference;

public class BREthereumToken extends BRCoreJniReference {

    private BREthereumToken (long jniReferenceAddress) {
        super (jniReferenceAddress);
    }

    public native String getAddress ();
    public native String getSymbol ();
    public native String getName ();
    public native String getDescription ();
    public native int getDecimals ();
    public native String getColorLeft ();
    public native String getColorRight ();

    public long getIdentifier () {
        return jniReferenceAddress;
    }

    //
    // Lookup with an Address (the Contract)
    //

    /**
     * Return the token having `address` or `null` is `address` is unknown.
     *
     * @param address
     * @return
     */
    public static BREthereumToken lookup (String address) {
        for (BREthereumToken token : tokens)
            if (address.equalsIgnoreCase(token.getAddress()))
                return token;
        return null;
    }

    /**
     * Return the token having `reference` or `null` if `reference` is unknown.
     *
     * @param reference
     * @return
     */
    protected static BREthereumToken lookupByReference (long reference) {
        for (BREthereumToken token : tokens)
            if (reference == token.jniReferenceAddress)
                return token;
        return null;
    }

    //
    // All Tokens
    //
    public static final BREthereumToken[] tokens;

    static {
        long[] references = jniTokenAll ();
        tokens = new BREthereumToken[references.length];
        for (int i = 0; i < references.length; i++)
            tokens[i] = new BREthereumToken(references[i]);
    }

    //
    // The BRD Token
    //
    public static BREthereumToken tokenBRD =
            BREthereumToken.lookupByReference(jniGetTokenBRD());

    protected static native long jniGetTokenBRD ();
    protected static native long[] jniTokenAll ();

    @Override
    public int hashCode() {
        return getAddress().toLowerCase().hashCode();
    }
}
