package com.breadwallet.crypto.blockchaindb.models;

import com.google.common.base.Optional;

import org.json.JSONException;
import org.json.JSONObject;

public class SubscriptionEndpoint {

    public static Optional<SubscriptionEndpoint> asSubscriptionEndpoint(JSONObject json) {
        try {
            String environment = json.getString("environment");
            String kind = json.getString("kind");
            String value = json.getString("value");

            return Optional.of(new SubscriptionEndpoint(environment, kind, value));

        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    private final String environment;
    private final String kind;
    private final String value;

    public SubscriptionEndpoint(String environment, String kind, String value) {
        this.environment = environment;
        this.kind = kind;
        this.value = value;
    }

    public String getEnvironment() {
        return environment;
    }

    public String getKind() {
        return kind;
    }

    public String getValue() {
        return value;
    }
}
