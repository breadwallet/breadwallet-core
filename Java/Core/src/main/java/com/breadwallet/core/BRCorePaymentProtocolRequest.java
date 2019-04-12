/*
 * BreadWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core;

public class BRCorePaymentProtocolRequest extends BRCoreJniReference {
    //
    //
    //
    public BRCorePaymentProtocolRequest(byte[] data) {
        super(createPaymentProtocolRequest(data));
    }

    public native String getNetwork();

    public native BRCoreTransactionOutput[] getOutputs ();

    public native long getTime ();

    public native long getExpires();

    public native String getMemo();

    public native String getPaymentURL ();

    public native byte[] getMerchantData ();

    public native long getVersion ();

    public native String getPKIType ();

    public native byte[] getPKIData ();

    public native byte[] getSignature ();

    public native byte[] getDigest ();

    public native byte[][] getCerts ();

    private static native long createPaymentProtocolRequest(byte[] data);

    public native byte[] serialize ();

    public native void disposeNative ();

    protected static native void initializeNative ();

    static { initializeNative(); }
}
