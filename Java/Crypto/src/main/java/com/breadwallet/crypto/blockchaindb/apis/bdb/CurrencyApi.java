package com.breadwallet.crypto.blockchaindb.apis.bdb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.models.bdb.Currency;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;

import java.util.List;

public class CurrencyApi {

    private final BdbApiClient jsonClient;

    public CurrencyApi(BdbApiClient jsonClient) {
        this.jsonClient = jsonClient;
    }

    public void getCurrencies(BlockchainCompletionHandler<List<Currency>> handler) {
        getCurrencies(null, handler);
    }

    public void getCurrencies(@Nullable String id, BlockchainCompletionHandler<List<Currency>> handler) {
        Multimap<String, String> params = id == null ? ImmutableMultimap.of() : ImmutableListMultimap.of(
                "blockchain_id", id);
        jsonClient.sendGetRequest("currencies", params, Currency::asCurrencies, handler);
    }

    public void getCurrency(String id, BlockchainCompletionHandler<Currency> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("currencies/%s", id);
        jsonClient.sendGetRequest(path, ImmutableListMultimap.of(), Currency::asCurrency, handler);
    }

}
