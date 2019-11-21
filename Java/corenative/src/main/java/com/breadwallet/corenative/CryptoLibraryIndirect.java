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
import com.sun.jna.Pointer;

public final class CryptoLibraryIndirect {

    private static final LibraryInterface INSTANCE = Native.load(CryptoLibrary.LIBRARY_NAME, LibraryInterface.class);

    // Can this be migrated to CryptoLibraryDirect? Well, not easily. The JNA library explicitly mentions
    // it doesn't support arrays of pointers in direct mapping mode. That said, it has an example of how
    // this can be done (see: com.sun.jna.StringArray).
    public static void cryptoNetworkSetNetworkFees(Pointer network, BRCryptoNetworkFee[] fees, SizeT count) {
        INSTANCE.cryptoNetworkSetNetworkFees(network, fees, count);
    }

    public interface LibraryInterface extends Library {

        // crypto/BRCryptoNetwork.h
        void cryptoNetworkSetNetworkFees(Pointer network, BRCryptoNetworkFee[] fees, SizeT count);
    }
}
