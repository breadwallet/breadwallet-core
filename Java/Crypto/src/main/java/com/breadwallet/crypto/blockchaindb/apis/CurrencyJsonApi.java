package com.breadwallet.crypto.blockchaindb.apis;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.Currency;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;

import static com.google.common.base.Preconditions.checkArgument;

public class CurrencyJsonApi {

    private final JsonApiClient jsonClient;

    public CurrencyJsonApi(JsonApiClient jsonClient) {
        this.jsonClient = jsonClient;
    }

    public void getCurrencies(BlockchainCompletionHandler<List<Currency>> handler) {
        getCurrencies(null, handler);
    }

    public void getCurrencies(@Nullable String id, BlockchainCompletionHandler<List<Currency>> handler) {
        Multimap<String, String> params = id == null ? ImmutableMultimap.of() : ImmutableListMultimap.of(
                "blockchain_id", id);
        jsonClient.makeRequest("currencies", params, new JsonApiCompletionArrayHandler() {
            @Override
            public void handleData(JSONArray json, boolean more) {
                checkArgument(!more);
                Optional<List<Currency>> currencies = Currency.asCurrencies(json);
                if (currencies.isPresent()) {
                    handler.handleData(currencies.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    public void getCurrency(String id, BlockchainCompletionHandler<Currency> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("currencies/%s", id);
        jsonClient.makeRequest(path, ImmutableListMultimap.of(), new JsonApiCompletionObjectHandler() {
            @Override
            public void handleData(JSONObject json, boolean more) {
                checkArgument(!more);
                Optional<Currency> currency = Currency.asCurrency(json);
                if (currency.isPresent()) {
                    handler.handleData(currency.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

}
