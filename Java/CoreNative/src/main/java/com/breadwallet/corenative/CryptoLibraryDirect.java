/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/18/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative;

import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.StringArray;

import java.nio.ByteBuffer;

public final class CryptoLibraryDirect {

    //
    // Crypto Core
    //

    // crypto/BRCryptoAccount.h
    public static native Pointer cryptoAccountCreate(ByteBuffer phrase, long timestamp);
    public static native Pointer cryptoAccountCreateFromSerialization(byte[] serialization, SizeT serializationLength);
    public static native long cryptoAccountGetTimestamp(Pointer account);
    public static native Pointer cryptoAccountGetFileSystemIdentifier(Pointer account);
    public static native Pointer cryptoAccountSerialize(Pointer account, SizeTByReference count);
    public static native int cryptoAccountValidateSerialization(Pointer account, byte[] serialization, SizeT count);
    public static native int cryptoAccountValidateWordsList(SizeT count);
    public static native Pointer cryptoAccountGeneratePaperKey(StringArray words);
    public static native int cryptoAccountValidatePaperKey(ByteBuffer phraseBuffer, StringArray wordsArray);
    public static native void cryptoAccountGive(Pointer obj);

    // crypto/BRCryptoAddress.h
    public static native Pointer cryptoAddressAsString(Pointer address);
    public static native int cryptoAddressIsIdentical(Pointer a1, Pointer a2);
    public static native void cryptoAddressGive(Pointer obj);

    // crypto/BRCryptoCurrency.h
    public static native Pointer cryptoCurrencyGetUids(Pointer currency);
    public static native Pointer cryptoCurrencyGetName(Pointer currency);
    public static native Pointer cryptoCurrencyGetCode(Pointer currency);
    public static native Pointer cryptoCurrencyGetType(Pointer currency);
    public static native Pointer cryptoCurrencyGetIssuer(Pointer currency);
    public static native int cryptoCurrencyIsIdentical(Pointer c1, Pointer c2);
    public static native void cryptoCurrencyGive(Pointer obj);

    // crypto/BRCryptoPrivate.h
    public static native Pointer cryptoCurrencyCreate(String uids, String name, String code, String type, String issuer);

    //
    // Crypto Primitives
    //

    // crypto/BRCryptoCipher.h
    public static native Pointer cryptoCipherCreateForAESECB(byte[] key, SizeT keyLen);
    public static native Pointer cryptoCipherCreateForChacha20Poly1305(Pointer key, byte[] nonce12, SizeT nonce12Len, byte[] ad, SizeT adLen);
    public static native Pointer cryptoCipherCreateForPigeon(Pointer privKey, Pointer pubKey, byte[] nonce12, SizeT nonce12Len);
    public static native SizeT cryptoCipherEncryptLength(Pointer cipher, byte[] src, SizeT srcLen);
    public static native int cryptoCipherEncrypt(Pointer cipher, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    public static native SizeT cryptoCipherDecryptLength(Pointer cipher, byte[] src, SizeT srcLen);
    public static native int cryptoCipherDecrypt(Pointer cipher, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    public static native void cryptoCipherGive(Pointer cipher);

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
