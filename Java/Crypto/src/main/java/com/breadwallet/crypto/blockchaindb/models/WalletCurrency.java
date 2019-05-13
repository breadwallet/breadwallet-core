package com.breadwallet.crypto.blockchaindb.models;

import com.breadwallet.crypto.blockchaindb.JsonUtilities;
import com.google.common.base.Optional;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class WalletCurrency {

    public static Optional<WalletCurrency> asWalletCurrency(JSONObject json) {
        try {
            String currency = json.getString("currency_id");
            List<String> addresses = JsonUtilities.getOptionalStringList(json, "address").or(Collections.emptyList());
            return Optional.of(new WalletCurrency(currency, addresses));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<WalletCurrency>> asWalletCurrencies(JSONArray json) {
        List<WalletCurrency> currencies = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject currencyObject = json.optJSONObject(i);
            if (currencyObject == null) {
                return Optional.absent();
            }

            Optional<WalletCurrency> optionalCurrency = WalletCurrency.asWalletCurrency(currencyObject);
            if (!optionalCurrency.isPresent()) {
                return Optional.absent();
            }

            currencies.add(optionalCurrency.get());
        }
        return Optional.of(currencies);
    }

    private final String currency;
    private final List<String> addresses;

    public WalletCurrency(String currency, List<String> addresses) {
        this.currency = currency;
        this.addresses = addresses;
    }

    public String getCurrency() {
        return currency;
    }

    public List<String> getAddresses() {
        return addresses;
    }
}
