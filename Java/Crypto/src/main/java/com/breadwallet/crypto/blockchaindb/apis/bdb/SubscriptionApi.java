/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Subscription;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableMultimap;

public class SubscriptionApi {

    private final BdbApiClient jsonClient;

    public SubscriptionApi(BdbApiClient jsonClient) {
        this.jsonClient = jsonClient;
    }

    public void getOrCreateSubscription(Subscription subscription, CompletionHandler<Subscription, QueryError> handler) {
        getSubscription(subscription.getId(), new CompletionHandler<Subscription, QueryError>() {
            @Override
            public void handleData(Subscription data) {
                handler.handleData(data);
            }

            @Override
            public void handleError(QueryError error) {
                createSubscription(subscription, handler);
            }
        });
    }

    public void createSubscription(Subscription subscription, CompletionHandler<Subscription, QueryError> handler) {
        jsonClient.sendPost("subscriptions", ImmutableMultimap.of(), Subscription.asJson(subscription),
                Subscription::asSubscription, handler);
    }

    public void getSubscription(String id, CompletionHandler<Subscription, QueryError> handler) {
        jsonClient.sendGetWithId("subscriptions", id, ImmutableMultimap.of(), Subscription::asSubscription, handler);
    }

    public void updateSubscription(Subscription subscription, CompletionHandler<Subscription, QueryError> handler) {
        jsonClient.sendPutWithId("subscriptions", subscription.getId(), ImmutableMultimap.of(),
                Subscription.asJson(subscription), Subscription::asSubscription, handler);
    }

    public void deleteSubscription(String id, CompletionHandler<Subscription, QueryError> handler) {
        jsonClient.sendDeleteWithId("subscriptions", id, ImmutableMultimap.of(), Subscription::asSubscription, handler);
    }
}
