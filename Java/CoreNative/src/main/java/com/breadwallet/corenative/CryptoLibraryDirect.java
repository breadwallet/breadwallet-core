package com.breadwallet.corenative;

import com.breadwallet.corenative.crypto.BRCryptoCWMListener;
import com.breadwallet.corenative.utility.SizeT;
import com.sun.jna.Native;
import com.sun.jna.Pointer;

public class CryptoLibraryDirect {

    public static final String LIBRARY_NAME = "crypto";

    // crypto/BRCryptoAddress.h
    public static native Pointer cryptoAddressAsString(Pointer address);
    public static native int cryptoAddressIsIdentical(Pointer a1, Pointer a2);
    public static native void cryptoAddressGive(Pointer obj);

    // crypto/BRCryptoCoder.h
    public static native Pointer cryptoCoderCreate(int type);
    public static native SizeT cryptoCoderEncodeLength(Pointer coder, byte[] src, SizeT srcLen);
    public static native int cryptoCoderEncode(Pointer coder, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    public static native SizeT cryptoCoderDecodeLength(Pointer coder, byte[] src);
    public static native int cryptoCoderDecode(Pointer coder, byte[] dst, SizeT dstLen, byte[] src);
    public static native void cryptoCoderGive(Pointer coder);

    public static native double cryptoWalletManagerCallMeMaybe(BRCryptoCWMListener.BRCryptoCWMListenerWalletManagerEvent callback, long count);
    public static native double cryptoWalletManagerCallMeMaybeDirect(BRCryptoCWMListener.BRCryptoCWMListenerWalletManagerEventDirect callback, long count);

    static {
        Native.register(CryptoLibraryDirect.class, CryptoLibrary.LIBRARY);
    }
}
