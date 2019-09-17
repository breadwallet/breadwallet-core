/*
 * EthereumToken
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 3/20/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core.ethereum;


import com.breadwallet.core.BRCoreJniReference;

import java.util.HashMap;

public class BREthereumToken extends BRCoreJniReference {

    protected BREthereumToken (long jniReferenceAddress) {
        super (jniReferenceAddress);
    }

    public native String getAddress ();
    public native String getSymbol ();
    public native String getName ();
    public native String getDescription ();
    public native int getDecimals ();

    public long getIdentifier () {
        return jniReferenceAddress;
    }


    protected static native long jniGetTokenBRD ();
    protected static native long[] jniTokenAll ();

    @Override
    public void dispose() {
        // avoid 'super.dispose()' and thus 'native.dispose()'
    }

    @Override
    public int hashCode() {
        return getAddress().toLowerCase().hashCode();
    }

    @Override
    public String toString() {
        return "BREthereumToken{" + getSymbol() + "}";
    }
}
