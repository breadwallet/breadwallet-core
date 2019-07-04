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
import java.nio.charset.StandardCharsets;
import java.util.Date;

import static com.google.common.base.Preconditions.checkArgument;

public interface CoreBRCryptoAccount {

    static CoreBRCryptoAccount create(String phrase) {
        byte[] phraseBytes = (phrase + "\0").getBytes(StandardCharsets.UTF_8);

        Memory phraseMemory = new Memory(phraseBytes.length);
        try {
            phraseMemory.write(0, phraseBytes, 0, phraseBytes.length);
            ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseBytes.length);

            return new OwnedBRCryptoAccount(CryptoLibrary.INSTANCE.cryptoAccountCreate(phraseBuffer));
        } finally {
            phraseMemory.clear();
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

    static byte[] deriveSeed(String phrase) {
        byte[] phraseBytes = (phrase + "\0").getBytes(StandardCharsets.UTF_8);

        Memory phraseMemory = new Memory(phraseBytes.length);
        try {
            phraseMemory.write(0, phraseBytes, 0, phraseBytes.length);
            ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseBytes.length);

            return CryptoLibrary.INSTANCE.cryptoAccountDeriveSeed(phraseBuffer).u8.clone();
        } finally {
            phraseMemory.clear();
        }
    }

    Date getTimestamp();

    void setTimestamp(Date timestamp);

    BRCryptoAccount asBRCryptoAccount();
}
