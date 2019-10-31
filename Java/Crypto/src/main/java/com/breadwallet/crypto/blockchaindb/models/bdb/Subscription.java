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

import org.json.JSONException;
import org.json.JSONObject;

public class Subscription {

    public static Optional<Subscription> asSubscription(JSONObject json) {
        try {
            String id = json.getString("subscription_id");
            String device = json.getString("device_id");

            JSONObject jsonEndpoint = json.getJSONObject("endpoint");
            Optional<SubscriptionEndpoint> optionalEndpoint = SubscriptionEndpoint.asSubscriptionEndpoint(jsonEndpoint);
            if (!optionalEndpoint.isPresent()) return Optional.absent();
            SubscriptionEndpoint endpoint= optionalEndpoint.get();

            return Optional.of(new Subscription(id, device, endpoint));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static JSONObject asJson(Subscription subscription) {
        return new JSONObject(ImmutableMap.of(
                "id", subscription.id,
                "device_id", subscription.device,
                "endpoint", SubscriptionEndpoint.asJson(subscription.endpoint)
        ));
    }

    private final String id;
    private final String device;
    private final SubscriptionEndpoint endpoint;

    public Subscription(String id, String device, SubscriptionEndpoint endpoint) {
        this.id = id;
        this.device = device;
        this.endpoint = endpoint;
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
}
