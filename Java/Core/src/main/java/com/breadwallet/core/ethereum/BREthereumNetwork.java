/*
 * EthereumNetwork
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 3/20/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core.ethereum;

import com.breadwallet.core.BRCoreJniReference;

public class BREthereumNetwork extends BRCoreJniReference {

    private BREthereumNetwork (long jniReferenceAddress) {
        super (jniReferenceAddress);
    }

    public long getIdentifier () {
        return jniReferenceAddress;
    }

    public static BREthereumNetwork mainnet =
            new BREthereumNetwork(jniGetMainnet());

    private static native long jniGetMainnet ();

    public static BREthereumNetwork testnet =
            new BREthereumNetwork(jniGetTestnet());

    private static native long jniGetTestnet ();

    public static BREthereumNetwork rinkeby =
            new BREthereumNetwork(jniGetRinkeby());

    private static native long jniGetRinkeby ();
}
