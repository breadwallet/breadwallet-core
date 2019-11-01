/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;
import com.google.common.collect.ImmutableMap;
import com.google.common.primitives.UnsignedInteger;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class SubscriptionEvent {

    public static Optional<SubscriptionEvent> asSubscriptionEvent(JSONObject json) {
        try {
            String name = json.getString("name");

            return Optional.of(new SubscriptionEvent(name, Collections.emptyList()));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<SubscriptionEvent>> asSubscriptionEvents(JSONArray json){
        List<SubscriptionEvent> subscriptionEvents = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject subscriptionEventObject = json.optJSONObject(i);
            if (subscriptionEventObject == null) {
                return Optional.absent();
            }

            Optional<SubscriptionEvent> optionalSubscriptionEvent = SubscriptionEvent.asSubscriptionEvent(subscriptionEventObject);
            if (!optionalSubscriptionEvent.isPresent()) {
                return Optional.absent();
            }

            subscriptionEvents.add(optionalSubscriptionEvent.get());
        }
        return Optional.of(subscriptionEvents);
    }

    public static JSONObject asJson(SubscriptionEvent event) {
        switch (event.name) {
            case "submitted":
                return new JSONObject(ImmutableMap.of(
                        "name", event.name
                ));
            case "confirmed":
                List<Long> confirmations = new ArrayList<>(event.confirmations.size());
                for (UnsignedInteger confirmation: event.confirmations) confirmations.add(confirmation.longValue());

                return new JSONObject(ImmutableMap.of(
                        "name", event.name,
                        "confirmations", confirmations
                ));
            default: throw new IllegalStateException("Invalid name");
        }
    }

    public static JSONArray asJson(List<SubscriptionEvent> events) {
        JSONArray array = new JSONArray();
        for (SubscriptionEvent event: events) {
            array.put(asJson(event));
        }
        return array;
    }

    private final String name;
    private final List<UnsignedInteger> confirmations;

    public SubscriptionEvent(String name, @Nullable List<UnsignedInteger> confirmations) {
        this.name = name;
        this.confirmations = confirmations;
    }

    public String getName() {
        return name;
    }

    public Optional<List<UnsignedInteger>> getConfirmations() {
        return Optional.fromNullable(confirmations);
    }
}
