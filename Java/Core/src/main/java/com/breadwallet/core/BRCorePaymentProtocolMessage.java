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

public class BRCorePaymentProtocolMessage extends BRCoreJniReference {
    public BRCorePaymentProtocolMessage (byte[] data) {
        this (createPaymentProtocolMessage (data));
    }

    public BRCorePaymentProtocolMessage (MessageType type, byte[] message,
                                         long statusCode, String statusMsg,
                                         byte[] identifier) {
        this (createPaymentProtocolMessageFull(type.value, message, statusCode, statusMsg, identifier));
    }

    protected BRCorePaymentProtocolMessage (long jniReferenceAddress) {
        super (jniReferenceAddress);
    }

    public MessageType getMessageType () {
        return MessageType.fromValue(getMessageTypeValue ());
    }

    private native int getMessageTypeValue ();

    public native byte[] getMessage ();

    public native long getStatusCode ();

    public native String getStatusMessage ();

    public native byte[] getIdentifier ();

    private static native long createPaymentProtocolMessage (byte[] data);

    private static native long createPaymentProtocolMessageFull (int type, byte[] message,
                                                                 long statusCode, String statusMsg,
                                                                 byte[] identifier);

    public native byte[] serialize ();

    public native void disposeNative ();

    //
    //
    //
    public enum MessageType {
        Unknown(0),
        InvoiceRequest(1),
        Request(2),
        Payment(3),
        ACK(4);

        protected int value;

        MessageType(int value) {
            this.value = value;
        }

        public static MessageType fromValue (int value) {
            for (MessageType type : MessageType.values())
                if (type.value == value)
                    return type;
            return Unknown;
        }
    }

}
