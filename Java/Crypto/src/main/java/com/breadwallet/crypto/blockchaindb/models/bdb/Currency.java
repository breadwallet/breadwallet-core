/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class Currency {

    private static final String ADDRESS_INTERNAL = "__native__";

    public static final String ADDRESS_BRD_MAINNET = "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6";
    public static final String ADDRESS_BRD_TESTNET = "0x7108ca7c4718efa810457f228305c9c71390931a";

    public static final String ADDRESS_EOS_MAINNET = "0x86fa049857e0209aa7d9e616f7eb3b3b78ecfdb0";

    public static Optional<Currency> asCurrency(JSONObject json) {
        // optional
        String address = json.optString("address", null);
        if (ADDRESS_INTERNAL.equals(address)) address = null;

        // required
        try {
            String id   = json.getString("currency_id");
            String name = json.getString("name");
            String code = json.getString("code");
            String type = json.getString("type");
            String blockchainId = json.getString("blockchain_id");

            JSONArray jsonDenominations = json.getJSONArray("denominations");
            Optional<List<CurrencyDenomination>> optionalDenominations = CurrencyDenomination.asDenominations(jsonDenominations);
            if (!optionalDenominations.isPresent()) return Optional.absent();
            List<CurrencyDenomination> denominations = optionalDenominations.get();

            boolean verified = json.getBoolean("verified");
            return Optional.of(new Currency(id, name, code, type, blockchainId, address, verified, denominations));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<Currency>> asCurrencies(JSONArray json){
        List<Currency> currencies = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject currencyObject = json.optJSONObject(i);
            if (currencyObject == null) {
                return Optional.absent();
            }

            Optional<Currency> optionalCurrency = Currency.asCurrency(currencyObject);
            if (!optionalCurrency.isPresent()) {
                return Optional.absent();
            }

            currencies.add(optionalCurrency.get());
        }
        return Optional.of(currencies);
    }

    private final String id;
    private final String name;
    private final String code;
    private final String type;
    private final String blockchainID;
    private final List<CurrencyDenomination> denominations;

    @Nullable
    private final String address;

    private final boolean verified;

    public Currency(String id, String name, String code, String type, String blockchainID, @Nullable String address, boolean verified,
                    List<CurrencyDenomination> denominations) {
        this.id = id;
        this.name = name;
        this.code = code;
        this.type = type;
        this.blockchainID = blockchainID;
        this.address = address;
        this.verified = verified;
        this.denominations = denominations;
    }

    public String getId() {
        return id;
    }

    public String getName() {
        return name;
    }

    public String getCode() {
        return code;
    }

    public String getType() {
        return type;
    }

    public String getBlockchainId() {
        return blockchainID;
    }

    public Optional<String> getAddress() {
        return Optional.fromNullable(address);
    }

    public boolean getVerified() {
        return verified;
    }

    public List<CurrencyDenomination> getDenominations() {
        return denominations;
    }
}
