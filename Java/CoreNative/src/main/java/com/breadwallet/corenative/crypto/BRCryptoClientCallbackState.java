/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoClientCallbackState extends PointerType {

    public BRCryptoClientCallbackState() {
        super();
    }

    public BRCryptoClientCallbackState(Pointer address) {
        super(address);
    }
}
