/*
 * BreadWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core;

import java.util.Arrays;

public class BRCoreTransactionInput extends BRCoreJniReference {

    /**
     * Create a transaction input.
     *
     * @param hash
     * @param index
     * @param amount
     * @param script
     * @param signature
     * @param sequence The sequence number.  If -1, TXIN_SEQUENCE will be used.
     */
    public BRCoreTransactionInput (byte[] hash, long index, long amount,
                                   byte[] script,
                                   byte[] signature,
                                   byte[] witness,
                                   long sequence) {
        this (createTransactionInput(hash, index, amount, script, signature, witness, sequence));
    }

    public BRCoreTransactionInput(long jniReferenceAddress) {
        super(jniReferenceAddress);
    }

    protected static native long createTransactionInput (byte[] hash, long index, long amount,
                                                         byte[] script,
                                                         byte[] signature,
                                                         byte[] witness,
                                                         long sequence);

    public native String getAddress ();

    private native void setAddress (String address);

    public native byte[] getHash ();

    public native long getIndex ();

    public native long getAmount ();

    public native byte[] getScript ();

    public native byte[] getSignature ();

    public native long getSequence ();
}

