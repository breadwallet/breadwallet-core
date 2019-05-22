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
        jsonClient.sendGetForArray("currencies", params, Currency::asCurrencies, handler);
    }

    public void getCurrency(String id, BlockchainCompletionHandler<Currency> handler) {
        jsonClient.sendGetWithId("currencies", id, ImmutableMultimap.of(), Currency::asCurrency, handler);
    }

}
