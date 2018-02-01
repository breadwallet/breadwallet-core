/*
 * BreadWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/30/18.
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


import java.util.Arrays;

public class BRCoreKey extends BRCoreJniReference {
    public BRCoreKey (byte[] privateKey) {
        this (createJniCoreKey());

        if (null == privateKey || 0 == privateKey.length)
            throw new NullPointerException("key is empty");

        if (!setPrivKey(privateKey)) {
            throw new IllegalArgumentException("Failed to setup the key: " + Arrays.toString(privateKey));
        }
    }

    public BRCoreKey (String secret) {
        this (createJniCoreKey());
        setSecret(secret.getBytes());
    }

    protected BRCoreKey (long jniReferenceAddress) {
        super (jniReferenceAddress);
    }

    public native byte[] getSecret ();
    public native byte[] getPubKey ();
    public native int    getCompressed ();

    private static native long createJniCoreKey ();

    public native boolean setPrivKey(byte[] privKey);

    private native void setSecret(byte[] secret);

    public native byte[] compactSign(byte[] data);

    public native byte[] encryptNative(byte[] data, byte[] nonce);

    public native byte[] decryptNative(byte[] data, byte[] nonce);

    public native String address();

    @Override
    public String toString() {
        return "BRCoreKey {@" + jniReferenceAddress +
                "\n  pubKey    : " + getPubKey() +
                "\n  compressed: " + getCompressed() +
                '}';
    }
}
