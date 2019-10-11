/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 9/9/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoPeer extends PointerType {

    public static Optional<BRCryptoPeer> create(BRCryptoNetwork network, String address, UnsignedInteger port, String publicKey) {
        return Optional.fromNullable(
                CryptoLibrary.INSTANCE.cryptoPeerCreate(network, address, port.shortValue(), publicKey)
        );
    }

    public BRCryptoPeer(Pointer address) {
        super(address);
    }

    public BRCryptoPeer() {
        super();
    }

    public BRCryptoNetwork getNetwork() {
        return CryptoLibrary.INSTANCE.cryptoPeerGetNetwork(this);
    }

    public String getAddress() {
        return CryptoLibrary.INSTANCE.cryptoPeerGetAddress(this).getString(0, "UTF-8");
    }

    public Optional<String> getPublicKey() {
        return Optional.fromNullable(CryptoLibrary.INSTANCE.cryptoPeerGetPublicKey(this)).transform(p -> p.getString(0, "UTF-8"));
    }

    public UnsignedInteger getPort() {
        return UnsignedInteger.fromIntBits(CryptoLibrary.INSTANCE.cryptoPeerGetPort(this));
    }

    public boolean isIdentical(BRCryptoPeer core) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoPeerIsIdentical(this, core);
    }

    public void give() {
        CryptoLibrary.INSTANCE.cryptoPeerGive(this);
    }
}
