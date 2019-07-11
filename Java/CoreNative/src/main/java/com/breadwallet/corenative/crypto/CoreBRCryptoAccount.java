/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.sun.jna.Memory;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Date;
import java.util.concurrent.TimeUnit;

import static com.google.common.base.Preconditions.checkArgument;

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

    static CoreBRCryptoAccount createFromSeed(byte[] seed, Date timestamp) {
        checkArgument(seed.length == 64);

        long timestampAsLong = TimeUnit.MILLISECONDS.toSeconds(timestamp.getTime());

        Memory seedMemory = new Memory(seed.length);
        try {
            seedMemory.write(0, seed, 0, seed.length);
            ByteBuffer seedBuffer = seedMemory.getByteBuffer(0, seed.length);

            return new OwnedBRCryptoAccount(CryptoLibrary.INSTANCE.cryptoAccountCreateFromSeedBytes(seedBuffer, timestampAsLong));
        } finally {
            seedMemory.clear();
        }
    }

    static byte[] deriveSeed(byte[] phraseUtf8) {
        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                return CryptoLibrary.INSTANCE.cryptoAccountDeriveSeed(phraseBuffer).u8.clone();
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    Date getTimestamp();

    BRCryptoAccount asBRCryptoAccount();
}
