/*
 * BreadWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 2/1/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core;

public class BRCorePaymentProtocolACK extends BRCoreJniReference {
    public BRCorePaymentProtocolACK(byte[] data) {
        this(createPaymentProtocolACK(data));
    }

    protected BRCorePaymentProtocolACK(long jniReferenceAddress) {
        super (jniReferenceAddress);
    }

    public native String getCustomerMemo ();

    public native byte[] getMerchantData ();

    public native BRCoreTransaction[] getTransactions ();

    public native BRCoreTransactionOutput[] getRefundTo ();

    public native String getMerchantMemo ();

    private static native long createPaymentProtocolACK(byte[] data);

    public native byte[] serialize ();

    public native void disposeNative ();

    protected static native void initializeNative ();

    static { initializeNative(); }
}
