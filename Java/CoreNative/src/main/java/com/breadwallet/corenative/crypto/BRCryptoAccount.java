/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.StringArray;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.concurrent.TimeUnit;

import static com.google.common.base.Preconditions.checkArgument;

public class BRCryptoAccount extends PointerType {

    public static BRCryptoAccount createFromPhrase(byte[] phraseUtf8, UnsignedLong timestamp, String uids) {
        long timestampAsLong = timestamp.longValue();

        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                return new BRCryptoAccount(
                        CryptoLibraryDirect.cryptoAccountCreate(
                                phraseBuffer,
                                timestampAsLong,
                                uids
                        )
                );
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    public static Optional<BRCryptoAccount> createFromSerialization(byte[] serialization, String uids) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoAccountCreateFromSerialization(
                        serialization,
                        new SizeT(serialization.length),
                        uids
                )
        ).transform(BRCryptoAccount::new);
    }

    public static byte[] generatePhrase(List<String> words) {
        checkArgument(BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoAccountValidateWordsList(new SizeT(words.size())));

        StringArray wordsArray = new StringArray(words.toArray(new String[0]), "UTF-8");

        Pointer phrasePtr = CryptoLibraryDirect.cryptoAccountGeneratePaperKey(wordsArray);
        try {
            return phrasePtr.getByteArray(0, (int) phrasePtr.indexOf(0, (byte) 0));
        } finally {
            Native.free(Pointer.nativeValue(phrasePtr));
        }
    }

    public static boolean validatePhrase(byte[] phraseUtf8, List<String> words) {
        checkArgument(BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoAccountValidateWordsList(new SizeT(words.size())));

        StringArray wordsArray = new StringArray(words.toArray(new String[0]), "UTF-8");

        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoAccountValidatePaperKey(phraseBuffer, wordsArray);
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    public BRCryptoAccount() {
        super();
    }

    public BRCryptoAccount(Pointer address) {
        super(address);
    }

    public Date getTimestamp() {
        Pointer thisPtr = this.getPointer();

        return new Date(TimeUnit.SECONDS.toMillis(CryptoLibraryDirect.cryptoAccountGetTimestamp(thisPtr)));
    }

    public String getUids() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoAccountGetUids(thisPtr).getString(0, "UTF-8");
    }

    public String getFilesystemIdentifier() {
        Pointer thisPtr = this.getPointer();

        Pointer ptr = CryptoLibraryDirect.cryptoAccountGetFileSystemIdentifier(thisPtr);
        try {
            return ptr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(ptr));
        }
    }

    public byte[] serialize() {
        Pointer thisPtr = this.getPointer();

        SizeTByReference bytesCount = new SizeTByReference();
        Pointer serializationPtr = CryptoLibraryDirect.cryptoAccountSerialize(thisPtr, bytesCount);
        try {
            return serializationPtr.getByteArray(0, UnsignedInts.checkedCast(bytesCount.getValue().longValue()));
        } finally {
            Native.free(Pointer.nativeValue(serializationPtr));
        }
    }

    public boolean validate(byte[] serialization) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoAccountValidateSerialization(thisPtr,
                serialization, new SizeT(serialization.length));
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoAccountGive(thisPtr);
    }
}
