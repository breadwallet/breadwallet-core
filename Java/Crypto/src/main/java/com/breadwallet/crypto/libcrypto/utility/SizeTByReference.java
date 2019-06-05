/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto.utility;

import com.google.common.primitives.UnsignedInts;
import com.sun.jna.Native;
import com.sun.jna.ptr.ByReference;
import com.sun.jna.ptr.IntByReference;

public class SizeTByReference extends ByReference {

    public SizeTByReference() {
        this(0);
    }

    public SizeTByReference(long value) {
        super(Native.SIZE_T_SIZE);
        setValue(value);
    }

    public void setValue(long value) {
        if (Native.SIZE_T_SIZE == 8) {
            getPointer().setLong(0, value);
        } else {
            getPointer().setInt(0, UnsignedInts.checkedCast(value));
        }
    }

    public long getValue() {
        if (Native.SIZE_T_SIZE == 8) {
            return getPointer().getLong(0);
        } else {
            return getPointer().getInt(0);
        }
    }
}
