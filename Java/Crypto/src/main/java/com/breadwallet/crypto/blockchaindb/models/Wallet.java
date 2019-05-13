package com.breadwallet.crypto.blockchaindb.models;

import com.breadwallet.crypto.blockchaindb.JsonUtilities;
import com.google.common.base.Optional;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Collections;
import java.util.Date;
import java.util.List;

public class Wallet {

    public static Optional<Wallet> asWallet(JSONObject json) {
        try {
            String id = json.getString("wallet_id");
            Date created = JsonUtilities.get8601DateFromString(json, "created");

            JSONArray jsonCurrencies = json.getJSONArray("currencies");
            Optional<List<WalletCurrency>> optionalCurrencies = WalletCurrency.asWalletCurrencies(jsonCurrencies);
            List<WalletCurrency> currencies = optionalCurrencies.or(Collections.emptyList());

            return Optional.of(new Wallet(id, created, currencies));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    private final String id;
    private final Date created;
    private final List<WalletCurrency> currencies;

    public Wallet(String id, Date created, List<WalletCurrency> currencies) {
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

    public List<WalletCurrency> getCurrencies() {
        return currencies;
    }
}
