package com.breadwallet.crypto.blockchaindb.models.bdb;

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
            "satoshi", "sat", 0, lookupSymbol("sat"));

    public static CurrencyDenomination BTC_BITCOIN = new CurrencyDenomination(
            "bitcoin", "btc", 8, lookupSymbol("btc"));


    public static CurrencyDenomination BCH_BITCOIN = new CurrencyDenomination(
            "bitcoin", "bch", 8, lookupSymbol("bch"));


    public static CurrencyDenomination ETH_WEI = new CurrencyDenomination(
            "wei", "wei", 0, lookupSymbol("wei"));

    public static CurrencyDenomination ETH_GWEI = new CurrencyDenomination(
            "gwei", "gwei", 9, lookupSymbol("gwei"));

    public static CurrencyDenomination ETH_ETHER = new CurrencyDenomination(
            "ether", "eth", 18, lookupSymbol("eth"));


    public static CurrencyDenomination BRD_INT = new CurrencyDenomination(
            "BRD_INTEGER", "BRDI", 0, lookupSymbol("brdi"));

    public static CurrencyDenomination BRD_BRD = new CurrencyDenomination(
            "BRD", "BRD", 18, lookupSymbol("brd"));


    public static CurrencyDenomination EOS_INT = new CurrencyDenomination(
            "EOS_INTEGER", "EOSI", 0, lookupSymbol("eosi"));

    public static CurrencyDenomination EOS_EOS = new CurrencyDenomination(
            "EOS", "EOS", 18, lookupSymbol("eos"));


    public static Optional<CurrencyDenomination> asDenomination(JSONObject json) {
        try {
            String name = json.getString("name");
            String code = json.getString("short_name");
            int decimals = json.getInt("decimals");
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
    private final int decimals;
    private final String symbol;

    public CurrencyDenomination(String name, String code, int decimals, String symbol) {
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

    public int getDecimals() {
        return decimals;
    }

    public String getSymbol() {
        return symbol;
    }
}
