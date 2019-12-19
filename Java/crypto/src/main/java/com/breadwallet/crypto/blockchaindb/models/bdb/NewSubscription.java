/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/15/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

public class NewSubscription {

    // creators

    @JsonCreator
    public static NewSubscription create(@JsonProperty("device_id") String deviceId,
                                         @JsonProperty("endpoint") SubscriptionEndpoint endpoint,
                                         @JsonProperty("currencies") List<SubscriptionCurrency> currencies) {
        return new NewSubscription(
                checkNotNull(deviceId),
                checkNotNull(endpoint),
                checkNotNull(currencies)
        );
    }

    // fields

    private final String deviceId;
    private final SubscriptionEndpoint endpoint;
    private final List<SubscriptionCurrency> currencies;

    private NewSubscription(String deviceId,
                            SubscriptionEndpoint endpoint,
                            List<SubscriptionCurrency> currencies) {
        this.deviceId = deviceId;
        this.endpoint = endpoint;
        this.currencies = currencies;
    }
    // getters

    @JsonProperty("device_id")
    public String getDevice() {
        return deviceId;
    }

    @JsonProperty("endpoint")
    public SubscriptionEndpoint getEndpoint() {
        return endpoint;
    }

    @JsonProperty("currencies")
    public List<SubscriptionCurrency> getCurrencies() {
        return currencies;
    }
}
