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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class Subscription {

    public static Optional<Subscription> asSubscription(JSONObject json) {
        try {
            String id = json.getString("subscription_id");
            String device = json.getString("device_id");

            JSONObject jsonEndpoint = json.getJSONObject("endpoint");
            Optional<SubscriptionEndpoint> optionalEndpoint = SubscriptionEndpoint.asSubscriptionEndpoint(jsonEndpoint);
            if (!optionalEndpoint.isPresent()) return Optional.absent();
            SubscriptionEndpoint endpoint= optionalEndpoint.get();

            JSONArray jsonCurrencies = json.getJSONArray("currencies");
            Optional<List<SubscriptionCurrency>> optionalCurrencies = SubscriptionCurrency.asSubscriptionCurrencies(jsonCurrencies);
            if (!optionalCurrencies.isPresent()) return Optional.absent();
            List<SubscriptionCurrency> currencies = optionalCurrencies.get();

            return Optional.of(new Subscription(id, device, endpoint, currencies));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<Subscription>> asSubscriptions(JSONArray json){
        List<Subscription> subscriptions = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject subscriptionObject = json.optJSONObject(i);
            if (subscriptionObject == null) {
                return Optional.absent();
            }

            Optional<Subscription> optionalSubscriptionCurrency = asSubscription(subscriptionObject);
            if (!optionalSubscriptionCurrency.isPresent()) {
                return Optional.absent();
            }

            subscriptions.add(optionalSubscriptionCurrency.get());
        }
        return Optional.of(subscriptions);
    }

    public static JSONObject asJson(Subscription subscription) {
        return new JSONObject(ImmutableMap.of(
                "id", subscription.id,
                "device_id", subscription.device,
                "endpoint", SubscriptionEndpoint.asJson(subscription.endpoint),
                "currencies", SubscriptionCurrency.asJson(subscription.currencies)
        ));
    }

    public static JSONObject asJson(String deviceId, SubscriptionEndpoint endpoint, List<SubscriptionCurrency> currencies) {
        return new JSONObject(ImmutableMap.of(
                "device_id", deviceId,
                "endpoint", SubscriptionEndpoint.asJson(endpoint),
                "currencies", SubscriptionCurrency.asJson(currencies)
        ));
    }

    private final String id;
    private final String device;
    private final SubscriptionEndpoint endpoint;
    private final List<SubscriptionCurrency> currencies;

    public Subscription(String id, String device, SubscriptionEndpoint endpoint, List<SubscriptionCurrency> currencies) {
        this.id = id;
        this.device = device;
        this.endpoint = endpoint;
        this.currencies = currencies;
    }

    public String getId() {
        return id;
    }

    public String getDevice() {
        return device;
    }

    public SubscriptionEndpoint getEndpoint() {
        return endpoint;
    }

    public List<SubscriptionCurrency> getCurrencies() {
        return currencies;
    }
}
