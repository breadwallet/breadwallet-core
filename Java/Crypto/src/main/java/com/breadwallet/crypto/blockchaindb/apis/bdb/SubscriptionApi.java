package com.breadwallet.crypto.blockchaindb.apis.bdb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Subscription;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableMultimap;

import org.json.JSONObject;

public class SubscriptionApi {

    private final BdbApiClient jsonClient;

    public SubscriptionApi(BdbApiClient jsonClient) {
        this.jsonClient = jsonClient;
    }

    public void getOrCreateSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        getSubscription(subscription.getId(), new BlockchainCompletionHandler<Subscription>() {
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

    public void createSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        jsonClient.sendPost("subscriptions", ImmutableMultimap.of(), Subscription.asJson(subscription),
                Subscription::asSubscription, handler);
    }

    public void getSubscription(String id, BlockchainCompletionHandler<Subscription> handler) {
        jsonClient.sendGetWithId("subscriptions", id, ImmutableMultimap.of(), Subscription::asSubscription, handler);
    }

    public void updateSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        jsonClient.sendPutWithId("subscriptions", subscription.getId(), ImmutableMultimap.of(),
                Subscription.asJson(subscription), Subscription::asSubscription, handler);
    }

    public void deleteSubscription(String id, BlockchainCompletionHandler<Subscription> handler) {
        jsonClient.sendDeleteWithId("subscriptions", id, ImmutableMultimap.of(), Subscription::asSubscription, handler);
    }
}
