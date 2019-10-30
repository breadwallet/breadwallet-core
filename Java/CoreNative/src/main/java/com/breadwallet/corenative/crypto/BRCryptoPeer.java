/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 9/9/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoPeer extends PointerType {

    public static Optional<BRCryptoPeer> create(BRCryptoNetwork network, String address, UnsignedInteger port, String publicKey) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoPeerCreate(
                        network.getPointer(),
                        address,
                        port.shortValue(),
                        publicKey
                )
        ).transform(BRCryptoPeer::new);
    }

    public BRCryptoPeer() {
        super();
    }

    public BRCryptoPeer(Pointer address) {
        super(address);
    }

    public BRCryptoNetwork getNetwork() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoNetwork(CryptoLibraryDirect.cryptoPeerGetNetwork(thisPtr));
    }

    public String getAddress() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoPeerGetAddress(thisPtr).getString(0, "UTF-8");
    }

    public Optional<String> getPublicKey() {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoPeerGetPublicKey(
                        thisPtr
                )
        ).transform(p -> p.getString(0, "UTF-8"));
    }

    public UnsignedInteger getPort() {
        Pointer thisPtr = this.getPointer();

        return UnsignedInteger.fromIntBits(CryptoLibraryDirect.cryptoPeerGetPort(thisPtr));
    }

    public boolean isIdentical(BRCryptoPeer other) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoPeerIsIdentical(thisPtr, other.getPointer());
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoPeerGive(thisPtr);
    }
}
