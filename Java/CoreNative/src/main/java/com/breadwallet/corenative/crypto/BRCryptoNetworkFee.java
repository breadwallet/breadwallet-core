/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoNetworkFee extends PointerType implements CoreBRCryptoNetworkFee {

    public BRCryptoNetworkFee(Pointer address) {
        super(address);
    }

    public BRCryptoNetworkFee() {
        super();
    }

    @Override
    public UnsignedLong getConfirmationTimeInMilliseconds() {
        return UnsignedLong.valueOf(CryptoLibrary.INSTANCE.cryptoNetworkFeeGetConfirmationTimeInMilliseconds(this));
    }

    @Override
    public boolean isIdentical(CoreBRCryptoNetworkFee other) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoNetworkFeeEqual(this, other.asBRCryptoNetworkFee());
    }

    @Override
    public BRCryptoNetworkFee asBRCryptoNetworkFee() {
        return this;
    }
}
