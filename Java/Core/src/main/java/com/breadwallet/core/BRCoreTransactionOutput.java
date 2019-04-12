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

public class BRCoreTransactionOutput extends BRCoreJniReference {

    public BRCoreTransactionOutput(long amount,
                                   byte[] script) {
        this(createTransactionOutput(amount, script));
    }

    public BRCoreTransactionOutput(long jniReferenceAddress) {
        super(jniReferenceAddress);
    }

    protected static native long createTransactionOutput(long amount,
                                                         byte[] script);

    public native String getAddress();

    private native void setAddress(String address);

    public native long getAmount();

    /**
     * Change the output amount - typically used after computing transaction fees.
     */
    public native void setAmount(long amount);

    public native byte[] getScript();
}
