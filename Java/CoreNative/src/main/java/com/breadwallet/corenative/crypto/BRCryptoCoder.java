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
import com.google.common.base.Optional;
import com.google.common.primitives.Ints;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import static com.google.common.base.Preconditions.checkState;

public class BRCryptoCoder extends PointerType {

    // these must mirror BRCryptoCoderType's enum values
    private static final int CRYPTO_CODER_HEX         = 0;
    private static final int CRYPTO_CODER_BASE58      = 1;
    private static final int CRYPTO_CODER_BASE58CHECK = 2;

    public static Optional<BRCryptoCoder> createHex() {
        return create(CRYPTO_CODER_HEX);
    }

    public static Optional<BRCryptoCoder> createBase58() {
        return create(CRYPTO_CODER_BASE58);
    }

    public static Optional<BRCryptoCoder> createBase58Check() {
        return create(CRYPTO_CODER_BASE58CHECK);
    }

    private static Optional<BRCryptoCoder> create(int alg) {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoCoderCreate(alg));
    }

    public BRCryptoCoder(Pointer address) {
        super(address);
    }

    public BRCryptoCoder() {
        super();
    }

    public String encode(byte[] input) {
        SizeT length = CryptoLibrary.INSTANCE.cryptoCoderEncodeLength(this, input, new SizeT(input.length));
        int lengthAsInt = Ints.checkedCast(length.longValue());
        checkState(0 != lengthAsInt);

        byte[] output = new byte[lengthAsInt];
        int result = CryptoLibrary.INSTANCE.cryptoCoderEncode(this, output, new SizeT(output.length), input, new SizeT(input.length));
        checkState(result == BRCryptoBoolean.CRYPTO_TRUE);
        return utf8BytesToString(output);
    }

    public Optional<byte[]> decode(String inputStr) {
        // ensure string is null terminated
        byte[] inputWithoutTerminator = inputStr.getBytes(StandardCharsets.UTF_8);
        byte[] inputWithTerminator = Arrays.copyOf(inputWithoutTerminator, inputWithoutTerminator.length + 1);

        SizeT length = CryptoLibrary.INSTANCE.cryptoCoderDecodeLength(this, inputWithTerminator);
        int lengthAsInt = Ints.checkedCast(length.longValue());
        if (0 == lengthAsInt) return Optional.absent();

        byte[] output = new byte[lengthAsInt];
        int result = CryptoLibrary.INSTANCE.cryptoCoderDecode(this, output, new SizeT(output.length), inputWithTerminator);
        checkState(result == BRCryptoBoolean.CRYPTO_TRUE);
        return Optional.of(output);
    }

    private static String utf8BytesToString(byte[] message) {
        int end = 0;
        int len = message.length;
        while ((end < len) && (message[end] != 0)) {
            end++;
        }
        return new String(message, 0, end, StandardCharsets.UTF_8);
    }

    public static class OwnedBRCryptoCoder extends BRCryptoCoder {

        public OwnedBRCryptoCoder(Pointer address) {
            super(address);
        }

        public OwnedBRCryptoCoder() {
            super();
        }

        @Override
        protected void finalize() {
            if (null != getPointer()) {
                CryptoLibrary.INSTANCE.cryptoCoderGive(this);
            }
        }
    }
}
