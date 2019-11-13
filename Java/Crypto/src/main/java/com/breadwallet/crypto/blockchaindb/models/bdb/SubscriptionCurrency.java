/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonProperty;

import java.util.List;
import java.util.Set;

public class SubscriptionCurrency {

    // creators

    public static SubscriptionCurrency create(String currencyId, List<String> addresses, List<SubscriptionEvent> events) {
        SubscriptionCurrency currency = new SubscriptionCurrency();
        currency.currencyId = currencyId;
        currency.addresses = addresses;
        currency.events = events;
        return currency;
    }

    // fields

    @JsonProperty("currency_id")
    private String currencyId;

    private List<String> addresses;

    private List<SubscriptionEvent> events;

    // getters

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
