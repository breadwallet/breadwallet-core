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

    public void getSubscription(String id, BlockchainCompletionHandler<Subscription> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("subscriptions/%s", id);
        makeSubscriptionRequest(null, path, "GET", handler);
    }

    public void createSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        makeSubscriptionRequest(subscription, "subscriptions", "POST", handler);
    }

    public void updateSubscription(Subscription subscription, BlockchainCompletionHandler<Subscription> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("subscriptions/%s", subscription.getId());
        makeSubscriptionRequest(subscription, path, "POST", handler);
    }

    public void deleteSubscription(String id, BlockchainCompletionHandler<Subscription> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("subscriptions/%s", id);
        makeSubscriptionRequest(null, path, "DELETE", handler);
    }

    private void makeSubscriptionRequest(@Nullable Subscription subscription, String path, String httpMethod,
                                         BlockchainCompletionHandler<Subscription> handler) {
        JSONObject json = subscription == null ? null : Subscription.asJson(subscription);
        jsonClient.sendRequest(path, ImmutableMultimap.of(), json, httpMethod, new ObjectCompletionHandler() {
            @Override
            public void handleData(JSONObject json, boolean more) {
                Optional<Subscription> optionalSubscription = Subscription.asSubscription(json);
                if (optionalSubscription.isPresent()) {
                    handler.handleData(optionalSubscription.get());
                } else {
                    handler.handleError(new QueryModelError("Missed subscription"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }
}
