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

import static com.google.common.base.Preconditions.checkArgument;

public interface CoreBRCryptoAccount {

    static CoreBRCryptoAccount createFromPhrase(byte[] phraseUtf8) {
        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);

        Memory phraseMemory = new Memory(phraseUtf8.length);
        try {
            phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
            ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

            return new OwnedBRCryptoAccount(CryptoLibrary.INSTANCE.cryptoAccountCreate(phraseBuffer));
        } finally {
            phraseMemory.clear();

            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    static CoreBRCryptoAccount createFromSeed(byte[] seed) {
        checkArgument(seed.length == 64);

        Memory seedMemory = new Memory(seed.length);
        try {
            seedMemory.write(0, seed, 0, seed.length);
            ByteBuffer seedBuffer = seedMemory.getByteBuffer(0, seed.length);

            return new OwnedBRCryptoAccount(CryptoLibrary.INSTANCE.cryptoAccountCreateFromSeedBytes(seedBuffer));
        } finally {
            seedMemory.clear();
        }
    }

    static byte[] deriveSeed(byte[] phraseUtf8) {
        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);

        Memory phraseMemory = new Memory(phraseUtf8.length);
        try {
            phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
            ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

            return CryptoLibrary.INSTANCE.cryptoAccountDeriveSeed(phraseBuffer).u8.clone();
        } finally {
            phraseMemory.clear();

            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    Date getTimestamp();

    void setTimestamp(Date timestamp);

    BRCryptoAccount asBRCryptoAccount();
}
