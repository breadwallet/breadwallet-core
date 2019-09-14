/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoAddress extends PointerType implements CoreBRCryptoAddress {

    public BRCryptoAddress(Pointer address) {
        super(address);
    }

    public BRCryptoAddress() {
        super();
    }

    public boolean isIdentical(CoreBRCryptoAddress o) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAddressIsIdentical(this, o.asBRCryptoAddress());
    }

    @Override
    public BRCryptoAddress asBRCryptoAddress() {
        return this;
    }

    @Override
    public String toString() {
        Pointer addressPtr = CryptoLibrary.INSTANCE.cryptoAddressAsString(this);
        try {
            return addressPtr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(addressPtr));
        }
    }
}
