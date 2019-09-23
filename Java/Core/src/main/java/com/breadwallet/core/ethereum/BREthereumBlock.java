/*
 * EthereumBlock
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 4/24/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core.ethereum;


public class BREthereumBlock extends BREthereumEWM.Reference {

    protected BREthereumBlock(BREthereumEWM ewm, long identifier) {
        super (ewm, identifier);
    }

//    public String getHash () {
//        return ewm.get().jniBlockGetHash(identifier);
//    }
//
//    public long getNumber () {
//        return ewm.get().jniBlockGetNumber(identifier);
//    }

//    public long getTimestamp () {
//        return node.get().jniBlockGetTimestamp(identifier);
//    }
}
