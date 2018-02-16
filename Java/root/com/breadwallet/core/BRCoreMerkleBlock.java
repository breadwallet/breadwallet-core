/*
 * BreadWallet
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 breadwallet LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
package com.breadwallet.core;

import java.util.Arrays;

/**
 *
 */
public class BRCoreMerkleBlock extends BRCoreJniReference {

    public BRCoreMerkleBlock(byte[] block, int blockHeight) {
        this (createJniCoreMerkleBlock (block, blockHeight));
    }

    protected BRCoreMerkleBlock (long jniReferenceAddress) {
        super (jniReferenceAddress);
    }

    private static native long createJniCoreMerkleBlock (byte[] block, int blockHeight);

    public native byte[] getBlockHash ();

    public native long getVersion ();

    public native byte[] getPrevBlockHash ();

    public native byte[] getRootBlockHash ();

    public native long getTimestamp ();

    public native long getTarget ();

    public native long getNonce ();

    public native long getTransactionCount ();

    // hashes

    // flags

    public native long getHeight ();

    /**
     * Serialize the transaction into a byte array.
     *
     * @return the byte array
     */
    public native byte[] serialize ();

    public native boolean isValid (long currentTime);

    public native boolean containsTransactionHash (byte[] hash);

    // verify difficulty

    // To call BRMerkleBlockFree()
    public native void disposeNative ();

    // From the DataSource get a BlockEntity; deconstruct and pass to PeerManager which produces
    // a C BRMerkleBlock; then, sometime later, get a saveBlock callback; lookup the BlockEntity
    // class and call the constructor - effectively converting a BRMerkleBlock back to a
    // BlockEntity.
    //
    // Whatever class (BRCoreWalletManager) is handling callbacks (like saveBlocks) will be getting
    // what - a BRMerkleBlock (C pointer)? - a BRCoreMerkleBlock (how)?
    //
    //
    // The Swift version does not use a subclass of BRMerkleBlock - it uses BRMerkleBlock directly;
    // but is also doesn't use a DataSource and instead uses SQLite queries directly.
    //
    // Perhaps the callbacks, like saveBlocks, needs some 'contextual parameter' to go from
    // BRMerkleBlock to whatever?  Surely must as one must go from BRMerkleBlock (in C) to
    // BRCoreMerkleBlock (in Java w/ reference to C).
    //
    // Callback contains BRCoreMerkleBlock and the JNI C callback code needs to construct them!!!

    @Override
    public String toString() {
        return "BRCoreMerkleBlock {@" + jniReferenceAddress +
                "\n  hash     : " + Arrays.toString(getBlockHash()) +
                "\n  timestamp: " + getTimestamp() +
                "\n  height   : " + getHeight() +
                "\n  txCount  : " + getTransactionCount() +
                "}";
    }
}
