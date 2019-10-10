/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.support.UInt256;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.sun.jna.FromNativeContext;
import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.NativeMapped;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.StringArray;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;

public class BRCryptoKey extends PointerType {

    public BRCryptoKey(Pointer address) {
        super(address);
    }

    public BRCryptoKey() {
        super();
    }

    public static boolean isProtectedPrivateKeyString(byte[] keyString) {
        // ensure string is null terminated
        keyString = Arrays.copyOf(keyString, keyString.length + 1);
        try {
            Memory keyMemory = new Memory(keyString.length);
            try {
                keyMemory.write(0, keyString, 0, keyString.length);
                ByteBuffer keyBuffer = keyMemory.getByteBuffer(0, keyString.length);

                return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoKeyIsProtectedPrivate(keyBuffer);
            } finally {
                keyMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(keyString, (byte) 0);
        }
    }

    public static Optional<BRCryptoKey> createFromPhrase(byte[] phraseUtf8, List<String> words) {
        StringArray wordsArray = new StringArray(words.toArray(new String[0]), "UTF-8");

        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoKeyCreateFromPhraseWithWords(phraseBuffer, wordsArray));
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    public static Optional<BRCryptoKey> createFromPrivateKeyString(byte[] keyString) {
        // ensure string is null terminated
        keyString = Arrays.copyOf(keyString, keyString.length + 1);
        try {
            Memory keyMemory = new Memory(keyString.length);
            try {
                keyMemory.write(0, keyString, 0, keyString.length);
                ByteBuffer keyBuffer = keyMemory.getByteBuffer(0, keyString.length);

                return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoKeyCreateFromStringPrivate(keyBuffer));
            } finally {
                keyMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(keyString, (byte) 0);
        }
    }

    public static Optional<BRCryptoKey> createFromPrivateKeyString(byte[] keyString, byte[] phraseString) {
        // ensure strings are null terminated
        keyString = Arrays.copyOf(keyString, keyString.length + 1);
        phraseString = Arrays.copyOf(phraseString, phraseString.length + 1);
        try {
            Memory memory = new Memory(keyString.length + phraseString.length);
            try {
                memory.write(0, keyString, 0, keyString.length);
                memory.write(keyString.length, phraseString, 0, phraseString.length);

                ByteBuffer keyBuffer = memory.getByteBuffer(0, keyString.length);
                ByteBuffer phraseBuffer = memory.getByteBuffer(keyString.length, phraseString.length);

                return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoKeyCreateFromStringProtectedPrivate(keyBuffer, phraseBuffer));
            } finally {
                memory.clear();
            }
        } finally {
            // clear out our copies; caller responsible for original arrays
            Arrays.fill(keyString, (byte) 0);
            Arrays.fill(phraseString, (byte) 0);
        }
    }

    public static Optional<BRCryptoKey> createFromPublicKeyString(byte[] keyString) {
        // ensure string is null terminated
        keyString = Arrays.copyOf(keyString, keyString.length + 1);
        try {
            Memory keyMemory = new Memory(keyString.length);
            try {
                keyMemory.write(0, keyString, 0, keyString.length);
                ByteBuffer keyBuffer = keyMemory.getByteBuffer(0, keyString.length);

                return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoKeyCreateFromStringPublic(keyBuffer));
            } finally {
                keyMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(keyString, (byte) 0);
        }
    }

    public static Optional<BRCryptoKey> createForPigeon(BRCryptoKey key, byte[] nonce) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoKeyCreateForPigeon(key, nonce, new SizeT(nonce.length)));
    }

    public static Optional<BRCryptoKey> createForBIP32ApiAuth(byte[] phraseUtf8, List<String> words) {
        StringArray wordsArray = new StringArray(words.toArray(new String[0]), "UTF-8");

        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoKeyCreateForBIP32ApiAuth(phraseBuffer, wordsArray));
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    public static Optional<BRCryptoKey> createForBIP32BitID(byte[] phraseUtf8, int index, String uri, List<String> words) {
        StringArray wordsArray = new StringArray(words.toArray(new String[0]), "UTF-8");

        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoKeyCreateForBIP32BitID(phraseBuffer, index, uri, wordsArray));

            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    public static Optional<BRCryptoKey> cryptoKeyCreateFromSecret(byte[] secret) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoKeyCreateFromSecret(new UInt256(secret).toByValue()));
    }

    public byte[] encodeAsPrivate() {
        Pointer ptr = CryptoLibrary.INSTANCE.cryptoKeyEncodePrivate(this);
        try {
            return ptr.getByteArray(0, (int) ptr.indexOf(0, (byte) 0));
        } finally {
            Native.free(Pointer.nativeValue(ptr));
        }
    }

    public byte[] encodeAsPublic() {
        Pointer ptr = CryptoLibrary.INSTANCE.cryptoKeyEncodePublic(this);
        try {
            return ptr.getByteArray(0, (int) ptr.indexOf(0, (byte) 0));
        } finally {
            Native.free(Pointer.nativeValue(ptr));
        }
    }

    public boolean hasSecret() {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoKeyHasSecret(this);
    }

    public byte[] getSecret() {
        return CryptoLibrary.INSTANCE.cryptoKeyGetSecret(this).u8;
    }

    public boolean privateKeyMatch(BRCryptoKey other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoKeySecretMatch(this, other);
    }

    public boolean publicKeyMatch(BRCryptoKey other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoKeyPublicMatch(this, other);
    }

    public void providePublicKey(int useCompressed, int compressed) {
        CryptoLibrary.INSTANCE.cryptoKeyProvidePublicKey(this, useCompressed, compressed);
    }

    public void give() {
        CryptoLibrary.INSTANCE.cryptoKeyGive(this);
    }
}
