/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.google.common.base.Optional;
import com.google.common.collect.ImmutableMap;
import com.google.common.primitives.UnsignedInteger;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class CurrencyDenomination {

    private static final Map<String, String> CURRENCY_SYMBOLS = ImmutableMap.of(
            "btc", "₿",
            "eth", "Ξ"
    );

    public static Optional<CurrencyDenomination> asDenomination(JSONObject json) {
        try {
            String name = json.getString("name");
            String code = json.getString("short_name");
            UnsignedInteger decimals = UnsignedInteger.valueOf(json.getLong("decimals"));
            String symbol = lookupSymbol(code);
            return Optional.of(new CurrencyDenomination(name, code, decimals, symbol));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<CurrencyDenomination>> asDenominations(JSONArray json){
        List<CurrencyDenomination> denominations = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject denominationsObject = json.optJSONObject(i);
            if (denominationsObject == null) {
                return Optional.absent();
            }

            Optional<CurrencyDenomination> optionalDenomination = CurrencyDenomination.asDenomination(denominationsObject);
            if (!optionalDenomination.isPresent()) {
                return Optional.absent();
            }

            denominations.add(optionalDenomination.get());
        }
        return Optional.of(denominations);
    }

    private static String lookupSymbol(String code) {
        String symbol = CURRENCY_SYMBOLS.get(code);
        return symbol != null ? symbol : code;
    }

    private final String name;
    private final String code;
    private final UnsignedInteger decimals;
    private final String symbol;

    public CurrencyDenomination(String name, String code, UnsignedInteger decimals, String symbol) {
        this.name = name;
        this.code = code;
        this.decimals = decimals;
        this.symbol = symbol;
    }

    public String getName() {
        return name;
    }

    public String getCode() {
        return code;
    }

    public UnsignedInteger getDecimals() {
        return decimals;
    }

    public String getSymbol() {
        return symbol;
    }
}
