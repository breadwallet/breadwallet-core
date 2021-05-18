/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.bdb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Currency;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;

import java.util.List;

public class CurrencyApi {

    private final BdbApiClient jsonClient;

    public CurrencyApi(BdbApiClient jsonClient) {
        this.jsonClient = jsonClient;
    }

    public void getCurrencies(CompletionHandler<List<Currency>, QueryError> handler) {
        getCurrencies(null, handler);
    }

    public void getCurrencies(@Nullable String id,
                              CompletionHandler<List<Currency>, QueryError> handler) {
        ImmutableListMultimap.Builder<String, String> paramsBuilder = ImmutableListMultimap.builder();
        if (id != null) paramsBuilder.put("blockchain_id", id);
        paramsBuilder.put("verified", "true");
        ImmutableMultimap<String, String> params = paramsBuilder.build();

        jsonClient.sendGetForArray("currencies", params, Currency.class, handler);
    }

    public void getCurrency(String id,
                            CompletionHandler<Currency, QueryError> handler) {
        jsonClient.sendGetWithId("currencies", id, ImmutableMultimap.of(), Currency.class, handler);
    }

}
