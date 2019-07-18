/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;

public interface CoreBRCryptoCurrency {

    static OwnedBRCryptoCurrency create(String uids, String name, String code, String type, String issuer) {
        return new OwnedBRCryptoCurrency(CryptoLibrary.INSTANCE.cryptoCurrencyCreate(uids, name, code, type, issuer));
    }

    String getUids();

    String getName();

    String getCode();

    String getType();

    boolean isIdentical(CoreBRCryptoCurrency coreBRCryptoCurrency);

    BRCryptoCurrency asBRCryptoCurrency();
}
