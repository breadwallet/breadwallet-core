/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/1/19.
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
import java.util.Collections;
import java.util.List;

public class SubscriptionCurrency {

    public static Optional<SubscriptionCurrency> asSubscriptionCurrency(JSONObject json) {
        try {
            String currency_id = json.getString("currency_id");

            JSONArray jsonAddresses = json.getJSONArray("addresses");
            List<String> addresses = new ArrayList<>(jsonAddresses.length());
            for (int i = 0; i < jsonAddresses.length(); i++) addresses.add(jsonAddresses.getString(i));

            JSONArray jsonEvents = json.getJSONArray("events");
            Optional<List<SubscriptionEvent>> optionalEvents = SubscriptionEvent.asSubscriptionEvents(jsonEvents);
            if (!optionalEvents.isPresent()) return Optional.absent();
            List<SubscriptionEvent> events = optionalEvents.get();

            return Optional.of(new SubscriptionCurrency(currency_id, addresses, events));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<SubscriptionCurrency>> asSubscriptionCurrencies(JSONArray json){
        List<SubscriptionCurrency> subscriptionCurrencies = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject subscriptionCurrencyObject = json.optJSONObject(i);
            if (subscriptionCurrencyObject == null) {
                return Optional.absent();
            }

            Optional<SubscriptionCurrency> optionalSubscriptionCurrency = asSubscriptionCurrency(subscriptionCurrencyObject);
            if (!optionalSubscriptionCurrency.isPresent()) {
                return Optional.absent();
            }

            subscriptionCurrencies.add(optionalSubscriptionCurrency.get());
        }
        return Optional.of(subscriptionCurrencies);
    }

    public static JSONObject asJson(SubscriptionCurrency currency) {
        return new JSONObject(ImmutableMap.of(
                    "currency_id", currency.currencyId,
                    "addresses", currency.addresses,
                    "events", SubscriptionEvent.asJson(currency.events)
            ));
    }

    public static JSONArray asJson(List<SubscriptionCurrency> currencies) {
        JSONArray array = new JSONArray();
        for (SubscriptionCurrency currency: currencies) {
            array.put(asJson(currency));
        }
        return array;
    }

    private final String currencyId;
    private final List<String> addresses;
    private final List<SubscriptionEvent> events;

    public SubscriptionCurrency(String currencyId, List<String> addresses, List<SubscriptionEvent> events) {
        this.currencyId = currencyId;
        this.addresses = addresses;
        this.events = events;
    }

    public String getCurrencyId() {
        return currencyId;
    }

    public List<String> getAddresses() {
        return addresses;
    }

    public List<SubscriptionEvent> getEvents() {
        return events;
    }
}
