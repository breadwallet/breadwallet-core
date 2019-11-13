/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonProperty;

import java.util.List;

public class Subscription {

    // creators

    public static Subscription create(String id, String deviceId, SubscriptionEndpoint endpoint, List<SubscriptionCurrency> currencies) {
        Subscription subscription = new Subscription();
        subscription.subscriptionId = id;
        subscription.deviceId = deviceId;
        subscription.endpoint = endpoint;
        subscription.currencies = currencies;
        return subscription;
    }

    public static Subscription create(String deviceId, SubscriptionEndpoint endpoint, List<SubscriptionCurrency> currencies) {
        Subscription subscription = new Subscription();
        subscription.deviceId = deviceId;
        subscription.endpoint = endpoint;
        subscription.currencies = currencies;
        return subscription;
    }

    // fields

    @JsonProperty("subscription_id")
    private String subscriptionId;

    @JsonProperty("device_id")
    private String deviceId;

    private SubscriptionEndpoint endpoint;

    private List<SubscriptionCurrency> currencies;

    // getters

    public String getId() {
        return subscriptionId;
    }

    public String getDevice() {
        return deviceId;
    }

    public SubscriptionEndpoint getEndpoint() {
        return endpoint;
    }

    public List<SubscriptionCurrency> getCurrencies() {
        return currencies;
    }
}
