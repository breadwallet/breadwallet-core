/*
 * BreadWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
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
package com.breadwallet.core;

/**
 *
 */
public class BRCoreTransaction extends BRCoreJniReference {
    //

    public BRCoreTransaction (byte[] buffer) {
        this (createJniCoreTransactionSerialized (buffer));
        // ...
    }

    public BRCoreTransaction (byte[] buffer, long blockHeight, long timeStamp ) {
        this (createJniCoreTransaction (buffer, blockHeight, timeStamp));
        // ...
    }

    protected BRCoreTransaction (long jniReferenceAddress) {
        super (jniReferenceAddress);
    }

    /**
     * Return the Transaction hash
     *
     * @return
     */
    public native byte[] getHash ();

    /**
     * The transaction's version
     *
     * @return the version is a long (from a uint32_t)
     */
    public native long getVersion ();

    /**
     *
     * @return
     */
    public native BRCoreTransactionInput[] getInputs ();

    /**
     *
     * @return
     */
    public native BRCoreTransactionOutput[] getOutputs ();

    /**
     * The transaction's lockTime
     *
     * @return the lock time as a long (from a uint32_t)
     */
    public native long getLockTime ();

    /**
     * The transaction's blockHeight.
     *
     * @return the blockHeight as a long (from a uint32_t).
     */
    public native long getBlockHeight ();

    /**
     * The transacdtion's timestamp.
     *
     * @return the timestamp as a long (from a uint32_t).
     */
    public native long getTimestamp ();

    // parse

    /**
     * Serialize the transaction into a byte array.
     *
     * @return the byte array
     */
    public native byte[] serialize ();

    /**
     *
     * @param input
     */
    public native void addInput (BRCoreTransactionInput input);

    /**
     *
     * @param output
     */
    public native void addOutput (BRCoreTransactionOutput output);

    /**
     * Shuffle the transaction's outputs.
     */
    public native void shuffleOutputs ();

    /**
     * The the transactions' size in bytes if signed, or the estimated size assuming
     * compact pubkey sigs

     * @return the size in bytes.
     */
    public native long getSize ();

    /**
     * The transaction's standard fee which is the minimum transaction fee needed for the
     * transaction to relay across the bitcoin network.
     * *
     * @return the fee (in Satoshis)?
     */
    public native long getStandardFee ();

    /**
     * Returns true if all the transaction's signatures exists.  This method does not verify
     * the signatures.
     *
     * @return true if all exist.
     */
    public native boolean isSigned ();

    // sign

    /**
     * Return true if this transaction satisfied the rules in:
     *      https://bitcoin.org/en/developer-guide#standard-transactions
     *
     * @return true if standard; false otherwise
     */
    public native boolean isStandard ();

    public static native long getMinOutputAmount ();

    /**
     * Call BRTransactionFree()
     */
    public native void disposeNative ();

    protected static native void initializeNative ();

    static { initializeNative(); }

    /**
     *
     * @param buffer
     * @param blockHeight
     * @param timeStamp
     * @return
     */
    private static native long createJniCoreTransaction (byte[] buffer, long blockHeight, long timeStamp);

    private static native long createJniCoreTransactionSerialized (byte[] buffer);

    @Override
    public String toString() {
        return "BRCoreTransaction {@" + jniReferenceAddress +
                "\n  hash       : " + getHash() +
                "\n  timestamp  : " + getTimestamp() +
                "\n  blockHeight: " + getBlockHeight() +
                "\n  standardFee: " + getStandardFee() +
                '}';
    }
}