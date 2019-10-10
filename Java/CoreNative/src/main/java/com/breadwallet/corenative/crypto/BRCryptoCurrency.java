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

public class BRCryptoCurrency extends PointerType {

    public static BRCryptoCurrency create(String uids, String name, String code, String type, String issuer) {
        return CryptoLibrary.INSTANCE.cryptoCurrencyCreate(uids, name, code, type, issuer);
    }

    public BRCryptoCurrency(Pointer address) {
        super(address);
    }

    public BRCryptoCurrency() {
        super();
    }

    public String getUids() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetUids(this).getString(0, "UTF-8");
    }

    public String getName() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetName(this).getString(0, "UTF-8");
    }

    public String getCode() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetCode(this).getString(0, "UTF-8");
    }

    public String getType() {
        return CryptoLibrary.INSTANCE.cryptoCurrencyGetType(this).getString(0, "UTF-8");
    }

    public String getIssuer() {
        Pointer issuer = CryptoLibrary.INSTANCE.cryptoCurrencyGetIssuer(this);
        return issuer == null ? null : issuer.getString(0, "UTF-8");
    }

    public boolean isIdentical(BRCryptoCurrency o) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoCurrencyIsIdentical(this, o);
    }

    public void give() {
        if (null != getPointer()) {
            CryptoLibrary.INSTANCE.cryptoCurrencyGive(this);
        }
    }
}
