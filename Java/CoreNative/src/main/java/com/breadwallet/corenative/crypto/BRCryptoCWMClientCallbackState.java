/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoCWMClientCallbackState extends PointerType {

    public BRCryptoCWMClientCallbackState(Pointer address) {
        super(address);
    }

    public BRCryptoCWMClientCallbackState() {
        super();
    }
}
