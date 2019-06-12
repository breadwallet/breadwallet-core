/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoHash extends PointerType implements CoreBRCryptoHash {

    public BRCryptoHash(Pointer address) {
        super(address);
    }

    public BRCryptoHash() {
        super();
    }

    @Override
    public int getValue() {
        return CryptoLibrary.INSTANCE.cryptoHashGetHashValue(this);
    }

    @Override
    public boolean isIdentical(CoreBRCryptoHash other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoHashEqual(this, other.asBRCryptoHash());
    }

    @Override
    public String toString() {
        return CryptoLibrary.INSTANCE.cryptoHashString(this).getString(0, "UTF-8");
    }

    @Override
    public BRCryptoHash asBRCryptoHash() {
        return this;
    }
}
