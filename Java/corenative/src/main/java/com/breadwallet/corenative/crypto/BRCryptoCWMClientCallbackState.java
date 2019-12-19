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

public class BRCryptoCWMClientCallbackState extends PointerType {

    public BRCryptoCWMClientCallbackState() {
        super();
    }

    public BRCryptoCWMClientCallbackState(Pointer address) {
        super(address);
    }
}
