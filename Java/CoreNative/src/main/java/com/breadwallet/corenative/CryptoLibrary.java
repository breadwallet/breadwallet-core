/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative;

import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.breadwallet.corenative.utility.SizeT;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Pointer;

public interface CryptoLibrary extends Library {

    String JNA_LIBRARY_NAME = "corecrypto";
    NativeLibrary LIBRARY = NativeLibrary.getInstance(CryptoLibrary.JNA_LIBRARY_NAME);
}
