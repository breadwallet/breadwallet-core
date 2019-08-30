/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
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


    public static CurrencyDenomination SATOSHI = new CurrencyDenomination(
            "Satoshi", "sat", UnsignedInteger.valueOf(0), lookupSymbol("sat"));


    public static CurrencyDenomination BTC_BITCOIN = new CurrencyDenomination(
            "Bitcoin", "btc", UnsignedInteger.valueOf(8), lookupSymbol("btc"));

    public static CurrencyDenomination BTC_BITCOIN_TESTNET = new CurrencyDenomination(
            "Bitcoin Testnet", "btc", UnsignedInteger.valueOf(8), lookupSymbol("btc"));


    public static CurrencyDenomination BCH_BITCOIN = new CurrencyDenomination(
            "Bitcoin Cash", "bch", UnsignedInteger.valueOf(8), lookupSymbol("bch"));

    public static CurrencyDenomination BCH_BITCOIN_TESTNET = new CurrencyDenomination(
            "Bitcoin Cash Testnet", "bch", UnsignedInteger.valueOf(8), lookupSymbol("bch"));


    public static CurrencyDenomination ETH_WEI = new CurrencyDenomination(
            "Wei", "wei", UnsignedInteger.valueOf(0), lookupSymbol("wei"));

    public static CurrencyDenomination ETH_GWEI = new CurrencyDenomination(
            "Gwei", "gwei", UnsignedInteger.valueOf(9), lookupSymbol("gwei"));

    public static CurrencyDenomination ETH_ETHER = new CurrencyDenomination(
            "Ether", "eth", UnsignedInteger.valueOf(18), lookupSymbol("eth"));


    public static CurrencyDenomination BRD_INT = new CurrencyDenomination(
            "BRD Token INT", "BRDI", UnsignedInteger.valueOf(0), lookupSymbol("brdi"));

    public static CurrencyDenomination BRD_BRD = new CurrencyDenomination(
            "BRD Token", "BRD", UnsignedInteger.valueOf(18), lookupSymbol("brd"));


    public static CurrencyDenomination EOS_INT = new CurrencyDenomination(
            "EOS_INTEGER", "EOSI", UnsignedInteger.valueOf(0), lookupSymbol("eosi"));

    public static CurrencyDenomination EOS_EOS = new CurrencyDenomination(
            "EOS", "EOS", UnsignedInteger.valueOf(18), lookupSymbol("eos"));


    public static CurrencyDenomination XRP_DROP = new CurrencyDenomination(
            "drop", "drop", UnsignedInteger.valueOf(0), lookupSymbol("drop"));

    public static CurrencyDenomination XRP_XRP = new CurrencyDenomination(
            "xrp", "xrp", UnsignedInteger.valueOf(6), lookupSymbol("xrp"));


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
