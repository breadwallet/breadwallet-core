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
public class BRCorePeer extends BRCoreJniReference {

    /**
     *
     */
    public enum ConnectStatus {
        Disconnected(0),
        Connecting(1),
        Connected(2),
        Unknown(-2);

        private int value;

        public int getValue () {
            return value;
        }

        ConnectStatus (int value) {
            this.value = value;
        }

        public static ConnectStatus fromValue (int value) {
            // Just a linear search - easy, quick-enough.
            for (ConnectStatus status : ConnectStatus.values())
                if (status.value == value)
                    return status;
            return Unknown;
        }
    }


    public String getName () {
        return getAddress() + ":" + getPort();
    }

    public native String getAddress ();

    public native int getPort ();

    public native void setEarliestKeyTime (long earliestKeyTime);

    public native void setCurrentBlockHeight (long currentBlockHeight);

    public ConnectStatus getConnectStatus () {
        return ConnectStatus.fromValue(getConnectStatusValue());
    }

    public native int getConnectStatusValue ();

    public native void connect ();

    public native void disconnect ();

    public native void scheduleDisconnect (double secoonds);

    public native void setNeedsFilterUpdate (boolean needsFilterUpdate);

    public native String getHost ();

    public native long getVersion ();

    public native String getUserAgent ();

    public native long getLastBlock ();

    public native long getFeePerKb ();

    public native double getPingTime ();

    // send message

    // send filter

    // send mempool

    // send getheaders

    // ...

    // send ping


    //
    //
    //
    public BRCorePeer(byte[] peerAddress,
                      byte[] peerPort,
                      byte[] timeStamp) {
        this(createJniCorePeer(peerAddress, peerPort, timeStamp));
    }

    public BRCorePeer (int magicNumber) {
        this (createJniCorePeer (magicNumber));
    }

    protected BRCorePeer(long jniReferenceAddress) {
        super(jniReferenceAddress);
    }

    private static native long createJniCorePeer(byte[] peerAddress,
                                                 byte[] peerPort,
                                                 byte[] timeStamp);

    private static native long createJniCorePeer(long magicNumber);



    @Override
    public String toString() {
        return "BRCorePeer{" +
                "jniReferenceAddress=" + jniReferenceAddress +
                '}';
    }
}
