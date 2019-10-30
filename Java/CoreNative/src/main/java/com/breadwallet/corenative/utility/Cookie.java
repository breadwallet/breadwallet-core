/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 18/10/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.utility;

import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public final class Cookie extends PointerType {

    public Cookie(Pointer cookie) {
        super(cookie);
    }

    public Cookie(int cookie) {
        this(Pointer.createConstant(cookie));
    }
}
