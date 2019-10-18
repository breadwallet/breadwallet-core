/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/18/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative;

import com.breadwallet.corenative.utility.SizeT;
import com.sun.jna.Native;
import com.sun.jna.Pointer;

public final class CryptoLibraryDirect {

    //
    // Crypto Primitives
    //

    // crypto/BRCryptoCoder.h
    public static native Pointer cryptoCoderCreate(int type);
    public static native SizeT cryptoCoderEncodeLength(Pointer coder, byte[] src, SizeT srcLen);
    public static native int cryptoCoderEncode(Pointer coder, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    public static native SizeT cryptoCoderDecodeLength(Pointer coder, byte[] src);
    public static native int cryptoCoderDecode(Pointer coder, byte[] dst, SizeT dstLen, byte[] src);
    public static native void cryptoCoderGive(Pointer coder);

    // crypto/BRCryptoHasher.h
    public static native Pointer cryptoHasherCreate(int type);
    public static native SizeT cryptoHasherLength(Pointer hasher);
    public static native int cryptoHasherHash(Pointer hasher, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    public static native void cryptoHasherGive(Pointer hasher);

    // crypto/BRCryptoSigner.h
    public static native Pointer cryptoSignerCreate(int type);
    public static native SizeT cryptoSignerSignLength(Pointer signer, Pointer key, byte[] digest, SizeT digestlen);
    public static native int cryptoSignerSign(Pointer signer, Pointer key, byte[] signature, SizeT signatureLen, byte[] digest, SizeT digestLen);
    public static native Pointer cryptoSignerRecover(Pointer signer, byte[] digest, SizeT digestLen, byte[] signature, SizeT signatureLen);
    public static native void cryptoSignerGive(Pointer signer);

    static {
        Native.register(CryptoLibraryDirect.class, CryptoLibrary.LIBRARY);
    }

    private CryptoLibraryDirect() {}
}
