/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.StringArray;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.concurrent.TimeUnit;

public interface CoreBRCryptoAccount {

    static CoreBRCryptoAccount createFromPhrase(byte[] phraseUtf8, Date timestamp) {
        long timestampAsLong = TimeUnit.MILLISECONDS.toSeconds(timestamp.getTime());

        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                return new OwnedBRCryptoAccount(CryptoLibrary.INSTANCE.cryptoAccountCreate(phraseBuffer, timestampAsLong));
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    static Optional<CoreBRCryptoAccount> createFromSerialization(byte[] serialization) {
        BRCryptoAccount account = CryptoLibrary.INSTANCE.cryptoAccountCreateFromSerialization(serialization, new SizeT(serialization.length));
        return Optional.fromNullable(account).transform(OwnedBRCryptoAccount::new);
    }

    static String generatePhrase(List<String> words) {
        StringArray wordsArray = new StringArray(words.toArray(new String[0]), "UTF-8");
        Pointer phrasePtr = CryptoLibrary.INSTANCE.cryptoAccountGeneratePaperKey(wordsArray);
        try {
            return phrasePtr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(phrasePtr));
        }
    }

    Date getTimestamp();

    byte[] serialize();

    BRCryptoAccount asBRCryptoAccount();
}
