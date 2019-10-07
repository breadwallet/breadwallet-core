/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoHash extends PointerType {

    public BRCryptoHash(Pointer address) {
        super(address);
    }

    public BRCryptoHash() {
        super();
    }

    public int getValue() {
        return CryptoLibrary.INSTANCE.cryptoHashGetHashValue(this);
    }

    public boolean isIdentical(BRCryptoHash other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoHashEqual(this, other);
    }

    @Override
    public String toString() {
        Pointer ptr = CryptoLibrary.INSTANCE.cryptoHashString(this);
        try {
            return ptr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(ptr));
        }
    }

    public static class OwnedBRCryptoHash extends BRCryptoHash {

        public OwnedBRCryptoHash(Pointer address) {
            super(address);
        }

        public OwnedBRCryptoHash() {
            super();
        }

        @Override
        protected void finalize() {
            if (null != getPointer()) {
                CryptoLibrary.INSTANCE.cryptoHashGive(this);
            }
        }
    }
}
