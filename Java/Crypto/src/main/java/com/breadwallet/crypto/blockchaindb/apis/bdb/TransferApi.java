/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transfer;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;

import java.util.List;

import static com.google.common.base.Preconditions.checkArgument;

public class TransferApi {

    private final BdbApiClient jsonClient;

    public TransferApi(BdbApiClient jsonClient) {
        this.jsonClient = jsonClient;
    }

    public void getTransfers(String id, List<String> addresses, CompletionHandler<List<Transfer>, QueryError> handler) {
        ImmutableListMultimap.Builder<String, String> paramBuilders = ImmutableListMultimap.builder();
        paramBuilders.put("blockchain_id", id);
        for (String address : addresses) paramBuilders.put("address", address);
        Multimap<String, String> params = paramBuilders.build();

        jsonClient.sendGetForArray("transfers", params, Transfer::asTransfers, handler);
    }

    public void getTransfer(String id, CompletionHandler<Transfer, QueryError> handler) {
        jsonClient.sendGetWithId("transfers", id, ImmutableMultimap.of(), Transfer::asTransfer, handler);
    }

}
