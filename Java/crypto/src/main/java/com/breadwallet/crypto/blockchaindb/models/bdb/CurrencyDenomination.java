/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.collect.ImmutableMap;
import com.google.common.primitives.UnsignedInteger;

import java.util.Locale;
import java.util.Map;

import static com.google.common.base.Preconditions.checkNotNull;

public class CurrencyDenomination {

    // creators

    public static CurrencyDenomination create(String name,
                                              String shortName,
                                              UnsignedInteger decimals,
                                              String symbol) {
        return new CurrencyDenomination(
                checkNotNull(name),
                checkNotNull(shortName),
                checkNotNull(decimals),
                checkNotNull(symbol)
        );
    }

    @JsonCreator
    public static CurrencyDenomination create(@JsonProperty("name") String name,
                                              @JsonProperty("short_name") String shortName,
                                              @JsonProperty("decimals") UnsignedInteger decimals) {
        return create(
                name,
                shortName,
                decimals,
                lookupSymbol(shortName)
        );
    }

    // fields

    private final String name;
    private final String shortName;
    private final UnsignedInteger decimals;
    private final String symbol;

    private CurrencyDenomination(String name,
                                 String shortName,
                                 UnsignedInteger decimals,
                                 String symbol) {
        this.name = name;
        this.shortName = shortName;
        this.decimals = decimals;
        this.symbol = symbol;
    }

    // getters

    @JsonProperty("name")
    public String getName() {
        return name;
    }

    @JsonProperty("short_name")
    public String getCode() {
        return shortName;
    }

    @JsonProperty("decimals")
    public UnsignedInteger getDecimals() {
        return decimals;
    }

    @JsonIgnore
    public String getSymbol() {
        return symbol == null ? lookupSymbol(shortName) : symbol;
    }

    // internals

    private static final Map<String, String> CURRENCY_SYMBOLS = ImmutableMap.of(
            "btc", "₿",
            "eth", "Ξ"
    );

    private static String lookupSymbol(String code) {
        String symbol = CURRENCY_SYMBOLS.get(code);
        return symbol != null ? symbol : code.toUpperCase(Locale.ROOT);
    }
}
