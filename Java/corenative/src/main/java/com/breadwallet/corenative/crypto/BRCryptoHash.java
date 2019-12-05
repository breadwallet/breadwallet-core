/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoHash extends PointerType {

    public BRCryptoHash() {
        super();
    }

    public BRCryptoHash(Pointer address) {
        super(address);
    }

    public int getValue() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoHashGetHashValue(thisPtr);
    }

    public boolean isIdentical(BRCryptoHash o) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoHashEqual(thisPtr, o.getPointer());
    }

    @Override
    public String toString() {
        Pointer thisPtr = this.getPointer();

        Pointer ptr = CryptoLibraryDirect.cryptoHashString(thisPtr);
        try {
            return ptr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(ptr));
        }
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoHashGive(thisPtr);
    }
}
