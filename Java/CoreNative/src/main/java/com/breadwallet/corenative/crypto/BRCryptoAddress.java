/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.google.common.base.Optional;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoAddress extends PointerType {

    public static Optional<BRCryptoAddress> create(String address, BRCryptoNetwork network) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoAddressCreateFromString(
                        network.getPointer(),
                        address
                )
        ).transform(BRCryptoAddress::new);
    }

    public BRCryptoAddress() {
        super();
    }

    public BRCryptoAddress(Pointer address) {
        super(address);
    }

    public boolean isIdentical(BRCryptoAddress o) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoAddressIsIdentical(thisPtr, o.getPointer());
    }

    @Override
    public String toString() {
        Pointer thisPtr = this.getPointer();

        Pointer addressPtr = CryptoLibraryDirect.cryptoAddressAsString(thisPtr);
        try {
            return addressPtr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(addressPtr));
        }
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoAddressGive(thisPtr);
    }
}
