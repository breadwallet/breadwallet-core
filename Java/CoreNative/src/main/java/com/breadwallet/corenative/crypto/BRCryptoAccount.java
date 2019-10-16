/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
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

    public static BRCryptoAccount createFromPhrase(byte[] phraseUtf8, UnsignedLong timestamp) {
        long timestampAsLong = timestamp.longValue();

        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                return CryptoLibrary.INSTANCE.cryptoAccountCreate(phraseBuffer, timestampAsLong);
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    public static Optional<BRCryptoAccount> createFromSerialization(byte[] serialization) {
        return Optional.fromNullable(
                CryptoLibrary.INSTANCE.cryptoAccountCreateFromSerialization(
                        serialization,
                        new SizeT(serialization.length)
                )
        );
    }

    public static byte[] generatePhrase(List<String> words) {
        checkArgument(BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAccountValidateWordsList(new SizeT(words.size())));

        StringArray wordsArray = new StringArray(words.toArray(new String[0]), "UTF-8");

        Pointer phrasePtr = CryptoLibrary.INSTANCE.cryptoAccountGeneratePaperKey(wordsArray);
        try {
            return phrasePtr.getByteArray(0, (int) phrasePtr.indexOf(0, (byte) 0));
        } finally {
            Native.free(Pointer.nativeValue(phrasePtr));
        }
    }

    public static boolean validatePhrase(byte[] phraseUtf8, List<String> words) {
        checkArgument(BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAccountValidateWordsList(new SizeT(words.size())));

        StringArray wordsArray = new StringArray(words.toArray(new String[0]), "UTF-8");

        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAccountValidatePaperKey(phraseBuffer, wordsArray);
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    public BRCryptoAccount(Pointer address) {
        super(address);
    }

    public BRCryptoAccount() {
        super();
    }

    public Date getTimestamp() {
        return new Date(TimeUnit.SECONDS.toMillis(CryptoLibrary.INSTANCE.cryptoAccountGetTimestamp(this)));
    }

    public String getFilesystemIdentifier() {
        Pointer ptr = CryptoLibrary.INSTANCE.cryptoAccountGetFileSystemIdentifier(this);
        try {
            return ptr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(ptr));
        }
    }

    public byte[] serialize() {
        SizeTByReference bytesCount = new SizeTByReference();
        Pointer serializationPtr = CryptoLibrary.INSTANCE.cryptoAccountSerialize(this, bytesCount);
        try {
            return serializationPtr.getByteArray(0, UnsignedInts.checkedCast(bytesCount.getValue().longValue()));
        } finally {
            Native.free(Pointer.nativeValue(serializationPtr));
        }
    }

    public boolean validate(byte[] serialization) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAccountValidateSerialization(this,
                serialization, new SizeT(serialization.length));
    }

    public void give() {
        CryptoLibrary.INSTANCE.cryptoAccountGive(this);
    }
}
