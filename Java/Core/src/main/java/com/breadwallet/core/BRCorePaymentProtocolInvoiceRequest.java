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

public class BRCorePaymentProtocolInvoiceRequest extends BRCoreJniReference {
    public BRCorePaymentProtocolInvoiceRequest(byte[] data) {
        super(createPaymentProtocolInvoiceRequest(data));
    }

    public BRCorePaymentProtocolInvoiceRequest (BRCoreKey senderPublicKey, long amount,
                                                String pkiType, byte[] pkiData,
                                                String memo, String notifyURL,
                                                byte[] signature) {
        super (createPaymentProtocolInvoiceRequestFull(senderPublicKey, amount,
                pkiType, pkiData,
                memo, notifyURL,
                signature));
    }

    public BRCoreKey getSenderPublicKey () {
        return new BRCoreKey (getSenderPublicKeyReference());
    }

    protected native long getSenderPublicKeyReference ();

    public native long getAmount ();

    public native String getPKIType ();

    public native byte[] getPKIData ();

    public native String getMemo ();

    public native String getNotifyURL ();

    public native byte[] getSignature ();

    private static native long createPaymentProtocolInvoiceRequest(byte[] data);

    private static native long createPaymentProtocolInvoiceRequestFull(BRCoreKey senderPublicKey, long amount,
                                                                        String pkiType, byte[] pkiData,
                                                                        String memo, String notifyURL,
                                                                        byte[] signature);

    public native byte[] serialize ();

    public native void disposeNative ();
}
