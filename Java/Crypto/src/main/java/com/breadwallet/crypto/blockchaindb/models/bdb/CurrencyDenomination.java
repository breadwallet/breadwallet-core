/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.collect.ImmutableMap;
import com.google.common.primitives.UnsignedInteger;

import java.util.Map;

public class CurrencyDenomination {

    // creators

    public static CurrencyDenomination create(String name, String shortName, UnsignedInteger decimals, String symbol) {
        CurrencyDenomination denomination = new CurrencyDenomination();
        denomination.name = name;
        denomination.shortName = shortName;
        denomination.decimals = decimals;
        denomination.symbol = symbol;
        return denomination;
    }

    // helpers

    private static final Map<String, String> CURRENCY_SYMBOLS = ImmutableMap.of(
            "btc", "₿",
            "eth", "Ξ"
    );

    private static String lookupSymbol(String code) {
        String symbol = CURRENCY_SYMBOLS.get(code);
        return symbol != null ? symbol : code;
    }

    // fields

    @JsonProperty("name")
    private String name;

    @JsonProperty("short_name")
    private String shortName;

    @JsonProperty("decimals")
    private UnsignedInteger decimals;

    @JsonIgnore
    private String symbol;

    // getters

    public String getName() {
        return name;
    }

    public String getCode() {
        return shortName;
    }

    public UnsignedInteger getDecimals() {
        return decimals;
    }

    public String getSymbol() {
        return symbol == null ? lookupSymbol(shortName) : symbol;
    }
}
