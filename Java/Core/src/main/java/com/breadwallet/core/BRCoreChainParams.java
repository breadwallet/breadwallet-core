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

/**
 *
 */
public class BRCoreChainParams extends BRCoreJniReference {

    private BRCoreChainParams (long jniReferenceAddress) {
        super (jniReferenceAddress);
    }

    //
    public native int getJniMagicNumber ();

    //
    //
    //

    public static BRCoreChainParams mainnetChainParams =
            new BRCoreChainParams(createJniMainnetChainParams());

    private static native long createJniMainnetChainParams ();

    public static BRCoreChainParams testnetChainParams =
            new BRCoreChainParams(createJniTestnetChainParams());

    private static native long createJniTestnetChainParams ();

    public static BRCoreChainParams mainnetBcashChainParams =
            new BRCoreChainParams(createJniMainnetBcashChainParams());

    private static native long createJniMainnetBcashChainParams();

    public static BRCoreChainParams testnetBcashChainParams =
            new BRCoreChainParams(createJniTestnetBcashChainParams());

    private static native long createJniTestnetBcashChainParams();
}
