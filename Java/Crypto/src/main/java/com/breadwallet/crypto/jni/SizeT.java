package com.breadwallet.crypto.jni;

import com.sun.jna.IntegerType;
import com.sun.jna.Native;

public class SizeT extends IntegerType {

    public SizeT() {
        this(0);
    }

    public SizeT(long value) {
        super(Native.SIZE_T_SIZE, value);
    }
}
