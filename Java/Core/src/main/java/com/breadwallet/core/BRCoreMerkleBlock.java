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

    // Test
    public BRCoreMerkleBlock () {
        this (createJniCoreMerkleBlockEmpty());
    }

    private static native long createJniCoreMerkleBlock (byte[] block, int blockHeight);

    // Test
    private static native long createJniCoreMerkleBlockEmpty ();

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

    /**
     * True if the given hash is known to be included in the block.  [The 'blockHash' is not
     * 'included in the block']
     *
     * @param hash
     * @return
     */
    public native boolean containsTransactionHash (byte[] hash);

    // verify difficulty

    // To call BRMerkleBlockFree()
    public native void disposeNative ();
}
