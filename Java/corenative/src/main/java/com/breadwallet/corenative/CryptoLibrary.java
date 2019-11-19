/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative;

import com.sun.jna.NativeLibrary;

public final class CryptoLibrary {

    public static final String LIBRARY_NAME;

    public static final NativeLibrary LIBRARY;

    static {
        LIBRARY_NAME = "test".equals(System.getProperty("com.breadwallet.corenative.libtype", null)) ?
                "corecryptoWithTests" : "corecrypto";
        LIBRARY = NativeLibrary.getInstance(CryptoLibrary.LIBRARY_NAME);
    }

    private CryptoLibrary() {}
}
