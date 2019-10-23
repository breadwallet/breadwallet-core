/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BRCryptoCurrency extends PointerType {

    public static BRCryptoCurrency create(String uids, String name, String code, String type, String issuer) {
        return new BRCryptoCurrency(CryptoLibraryDirect.cryptoCurrencyCreate(uids, name, code, type, issuer));
    }

    public BRCryptoCurrency(Pointer address) {
        super(address);
    }

    public BRCryptoCurrency() {
        super();
    }

    public String getUids() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoCurrencyGetUids(thisPtr).getString(0, "UTF-8");
    }

    public String getName() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoCurrencyGetName(thisPtr).getString(0, "UTF-8");
    }

    public String getCode() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoCurrencyGetCode(thisPtr).getString(0, "UTF-8");
    }

    public String getType() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoCurrencyGetType(thisPtr).getString(0, "UTF-8");
    }

    public String getIssuer() {
        Pointer thisPtr = this.getPointer();

        Pointer issuer = CryptoLibraryDirect.cryptoCurrencyGetIssuer(thisPtr);
        return issuer == null ? null : issuer.getString(0, "UTF-8");
    }

    public boolean isIdentical(BRCryptoCurrency o) {
        Pointer thisPtr = this.getPointer();

        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoCurrencyIsIdentical(thisPtr, o.getPointer());
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoCurrencyGive(thisPtr);
    }
}
