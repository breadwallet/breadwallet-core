/*
 * EthereumLightNode
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 3/7/18.
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
package com.breadwallet.core.ethereum;


/**
 *
 */
public class BREthereumLightNode
{
    static class Account {
	long id;
	private Account (long id) {
	    this.id = id;
	}
    }

    long jniReferenceAddress;

    public BREthereumLightNode () {
	this.jniReferenceAddress = jniCreateEthereumLightNode ();
    }

    public Account createAccount (String paperKey) {
	return new Account (jniCreateEthereumLightNodeAccount (paperKey));
    }

    public String getAccountPrimaryAddress (Account account) {
	return jniGetAccountPrimaryAddress (account.id);
    }

    protected static native long jniCreateEthereumLightNode ();
    protected native long jniCreateEthereumLightNodeAccount (String paperKey);
    protected native String jniGetAccountPrimaryAddress (long account);


    //    public native void disposeNative ();

    protected static native void initializeNative ();

    static { initializeNative(); }
}

