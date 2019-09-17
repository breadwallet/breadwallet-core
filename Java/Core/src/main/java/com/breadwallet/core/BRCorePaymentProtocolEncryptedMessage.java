/*
 * BreadWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core;


public class BRCorePaymentProtocolEncryptedMessage extends BRCoreJniReference {
    public BRCorePaymentProtocolEncryptedMessage (byte[] data) {
        super (createPaymentProtocolEncryptedMessage (data));
    }

    public native byte[] getMessage ();

    public BRCoreKey getReceiverPublicKey () {
        return new BRCoreKey (getReceiverPublicKeyReference());
    }

    public native long getReceiverPublicKeyReference ();

    public BRCoreKey getSenderPublicKey () {
        return new BRCoreKey (getSenderPublicKeyReference());
    }

    public native long getSenderPublicKeyReference ();

    public native long getNonce ();

    public native byte[] getSignature ();

    public native byte[] getIdentifier ();

    public native long getStatusCode ();

    public native String getStatusMessage ();

    private static native long createPaymentProtocolEncryptedMessage (byte[] data);

    public native byte[] serialize ();

    public native void disposeNative ();
}
