package com.breadwallet.crypto.blockchaindb.models;

import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Date;
import java.util.List;
import java.util.Map;

public class Wallet {

    public static Optional<Wallet> asWallet(JSONObject json) {
        try {
            String id = json.getString("wallet_id");
            Date created = Utilities.get8601DateFromString(json, "created");

            ImmutableMap.Builder<String, List<String>> currenciesBuilder = new ImmutableMap.Builder<>();
            JSONObject currenciesJson = json.getJSONObject("currencies");
            for (String currency: (Iterable<String>) () -> currenciesJson.keys()) {

                ImmutableList.Builder<String> addressesBuilder = new ImmutableList.Builder<>();
                JSONArray currencyAddressesJson = currenciesJson.getJSONArray(currency);
                for (int i = 0; i < currencyAddressesJson.length(); i++) {
                    String currencyAddress = currencyAddressesJson.getString(i);
                    addressesBuilder.add(currency);
                }

                currenciesBuilder.put(currency, addressesBuilder.build());
            }

            return Optional.of(new Wallet(id, created, currenciesBuilder.build()));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static JSONObject asJson(Wallet wallet) {
        return new JSONObject(ImmutableMap.of(
                "id", wallet.id,
                "created", Utilities.get8601StringFromDate(wallet.created),
                "currencies", wallet.currencies
                ));
    }

    private final String id;
    private final Date created;
    private final Map<String, List<String>> currencies;

    public Wallet(String id, Date created, Map<String, List<String>> currencies) {
        this.id = id;
        this.created = created;
        this.currencies = currencies;
    }

    public String getId() {
        return id;
    }

    public Date getCreated() {
        return created;
    }
}
