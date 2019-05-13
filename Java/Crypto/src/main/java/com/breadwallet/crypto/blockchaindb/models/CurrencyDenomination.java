package com.breadwallet.crypto.blockchaindb.models;

import com.google.common.base.Optional;
import com.google.common.collect.ImmutableMap;

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

    public static CurrencyDenomination BTC_SATOSHI = new CurrencyDenomination(
            "satoshi", "sat", (byte) 0, lookupSymbol("sat"));

    public static CurrencyDenomination BTC_BITCOIN = new CurrencyDenomination(
            "bitcoin", "btc", (byte) 8, lookupSymbol("btc"));


    public static CurrencyDenomination BCH_BITCOIN = new CurrencyDenomination(
            "bitcoin", "bch", (byte) 8, lookupSymbol("bch"));


    public static CurrencyDenomination ETH_WEI = new CurrencyDenomination(
            "wei", "wei", (byte) 0, lookupSymbol("wei"));

    public static CurrencyDenomination ETH_GWEI = new CurrencyDenomination(
            "gwei", "gwei", (byte) 9, lookupSymbol("gwei"));

    public static CurrencyDenomination ETH_ETHER = new CurrencyDenomination(
            "ether", "eth", (byte) 18, lookupSymbol("eth"));


    public static CurrencyDenomination BRD_INT = new CurrencyDenomination(
            "BRD_INTEGER", "BRDI", (byte) 0, lookupSymbol("brdi"));

    public static CurrencyDenomination BRD_BRD = new CurrencyDenomination(
            "BRD", "BRD", (byte) 18, lookupSymbol("brd"));


    public static CurrencyDenomination EOS_INT = new CurrencyDenomination(
            "EOS_INTEGER", "EOSI", (byte) 0, lookupSymbol("eosi"));

    public static CurrencyDenomination EOS_EOS = new CurrencyDenomination(
            "EOS", "EOS", (byte) 18, lookupSymbol("eos"));


    public static Optional<CurrencyDenomination> asDenomination(JSONObject json) {
        try {
            String name = json.getString("name");
            String code = json.getString("short_name");
            byte decimals = (byte) json.getInt("decimals"); // TODO: This needs to be rethought
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
    private final byte decimals;
    private final String symbols;

    public CurrencyDenomination(String name, String code, byte decimals, String symbols) {
        this.name = name;
        this.code = code;
        this.decimals = decimals;
        this.symbols = symbols;
    }

    public String getName() {
        return name;
    }

    public String getCode() {
        return code;
    }

    public byte getDecimals() {
        return decimals;
    }

    public String getSymbols() {
        return symbols;
    }
}
