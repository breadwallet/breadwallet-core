/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoCurrency extends PointerType implements CoreBRCryptoCurrency {

    public BRCryptoCurrency(Pointer address) {
        super(address);
    }

    public BRCryptoCurrency() {
        super();
    }

    @Override
    public String getUids() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetUids(this).getString(0, "UTF-8");
    }

    @Override
    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetName(this).getString(0, "UTF-8");
    }

    @Override
    public String getCode() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetCode(this).getString(0, "UTF-8");
    }

    @Override
    public String getType() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetType(this).getString(0, "UTF-8");
    }

    @Override
    public String getIssuer() {
        Pointer issuer = CryptoLibrary.INSTANCE.cryptoCurrencyGetIssuer(this);
        return issuer == null ? null : issuer.getString(0, "UTF-8");
    }

    @Override
    public boolean isIdentical(CoreBRCryptoCurrency coreBRCryptoCurrency) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoCurrencyIsIdentical(this, coreBRCryptoCurrency.asBRCryptoCurrency());
    }

    @Override
    public BRCryptoCurrency asBRCryptoCurrency() {
        return this;
    }
}
