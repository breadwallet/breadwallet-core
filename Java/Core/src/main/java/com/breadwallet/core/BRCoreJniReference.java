/*
 * BreadWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core;

/**
 *
 */
public abstract class BRCoreJniReference {

    static {
        try { System.loadLibrary("core"); }
        catch (UnsatisfiedLinkError e) {
            e.printStackTrace();
            System.err.println ("Native code library failed to load.\\n\" + " + e);
        }
    }

    protected static boolean SHOW_FINALIZE = false;
    /**
     * C Pointer (as a Java long) to the underlying Breadwallet Core entity allocated from the
     * C heap memory.  The referenced Core entity is used to implement native functions that
     * call Core functions (and thus expect a Core entity).
     *
     * The address must be determined in a subclass specific way and thus must be provided in the
     * subclasses constructor.
     */
    protected long jniReferenceAddress;

    protected BRCoreJniReference (long jniReferenceAddress)
    {
        this.jniReferenceAddress = jniReferenceAddress;
    }

    //
    //
    //
    protected void finalize () throws Throwable {
        if (SHOW_FINALIZE) System.err.println("Finalize: " + toString());
        dispose ();
    }

    public void dispose () {
        disposeNative ();
    }

    public native void disposeNative ();

    public String toString() {
        return getClass().getName() + "@" + Integer.toHexString(hashCode()) + " JNI=" + Long.toHexString(jniReferenceAddress);
    }
}
