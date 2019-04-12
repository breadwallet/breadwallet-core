/*
 * Currency
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

public class Currency {
    public final String code;
    public final String symbol;
    public final String name;
    public final int decimals;

    public final Unit baseUnit;
    public final Unit defaultUnit;

    public Currency(String code, String symbol, String name, int decimals, String baseUnitName, String baseUnitSymbol) {
        this.code = code;
        this.symbol = symbol;
        this.name = name;
        this.decimals = decimals;
        this.baseUnit    = new Unit (this, baseUnitName, baseUnitSymbol);
        this.defaultUnit = new Unit (code, symbol, (long) Math.pow (10, decimals), this.baseUnit);
    }
}
