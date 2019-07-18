/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.CompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryResponseError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Lists;
import com.google.common.collect.Multimap;
import com.google.common.io.BaseEncoding;
import com.google.common.primitives.UnsignedLong;
import com.google.common.primitives.UnsignedLongs;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Semaphore;

public class TransactionApi {

    private static final UnsignedLong PAGINATION_COUNT = UnsignedLong.valueOf(5000);
    private static final int ADDRESS_COUNT = 100;

    private final BdbApiClient jsonClient;
    private final ExecutorService executorService;

    public TransactionApi(BdbApiClient jsonClient, ExecutorService executorService) {
        this.jsonClient = jsonClient;
        this.executorService = executorService;
    }

    public void getTransactions(String id, List<String> addresses, UnsignedLong beginBlockNumber, UnsignedLong endBlockNumber,
                                boolean includeRaw, boolean includeProof,
                                CompletionHandler<List<Transaction>> handler) {
        executorService.submit(() -> getTransactionsOnExecutor(id, addresses, beginBlockNumber, endBlockNumber,
                includeRaw, includeProof, handler));
    }

    public void getTransaction(String id, boolean includeRaw, boolean includeProof,
                               CompletionHandler<Transaction> handler) {
        Multimap<String, String> params = ImmutableListMultimap.of("include_proof", String.valueOf(includeProof),
                "include_raw", String.valueOf(includeRaw));

        jsonClient.sendGetWithId("transactions", id, params, Transaction::asTransaction, handler);
    }

    public void createTransaction(String id, String hashAsHex, byte[] tx, CompletionHandler<Void> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "blockchain_id", id,
                "transaction_id", hashAsHex,
                "data", BaseEncoding.base64().encode(tx)));
        jsonClient.sendPost("transactions", ImmutableMultimap.of(), json, Optional::of, new CompletionHandler<Object>() {
            @Override
            public void handleData(Object data) {
                handler.handleData(null);
            }

            @Override
            public void handleError(QueryError error) {
                if (error instanceof QueryResponseError) {
                    int statusCode = ((QueryResponseError) error).getStatusCode();
                    // Consider 302 or 404 errors as success - owing to an issue with transaction submission
                    if (statusCode == 302 || statusCode == 404) {
                        handler.handleData(null);
                    } else {
                        handler.handleError(error);
                    }
                } else {
                    handler.handleError(error);
                }
            }
        });
    }

    private void getTransactionsOnExecutor(String id, List<String> addresses, UnsignedLong beginBlockNumber,
                                           UnsignedLong endBlockNumber, boolean includeRaw, boolean includeProof,
                                           CompletionHandler<List<Transaction>> handler) {
        final QueryError[] error = {null};
        List<Transaction> allTransactions = new ArrayList<>();
        Semaphore sema = new Semaphore(0);

        List<List<String>> chunkedAddressesList = Lists.partition(addresses, ADDRESS_COUNT);
        for (int i = 0; i < chunkedAddressesList.size() && error[0] == null; i++) {
            List<String> chunkedAddresses = chunkedAddressesList.get(i);

            ImmutableListMultimap.Builder<String, String> baseBuilder = ImmutableListMultimap.builder();
            baseBuilder.put("blockchain_id", id);
            baseBuilder.put("include_proof", String.valueOf(includeProof));
            baseBuilder.put("include_raw", String.valueOf(includeRaw));
            for (String address : chunkedAddresses) baseBuilder.put("address", address);
            ImmutableMultimap<String, String> baseParams = baseBuilder.build();

            for (UnsignedLong j = beginBlockNumber; j.compareTo(endBlockNumber) < 0 && error[0] == null; j = j.plus(PAGINATION_COUNT)) {
                ImmutableListMultimap.Builder<String, String> paramsBuilder = ImmutableListMultimap.builder();
                paramsBuilder.putAll(baseParams);
                paramsBuilder.put("start_height", j.toString());
                paramsBuilder.put("end_height", UnsignedLongs.toString(UnsignedLongs.min(j.plus(PAGINATION_COUNT).longValue(), endBlockNumber.longValue())));
                ImmutableMultimap<String, String> params = paramsBuilder.build();

                jsonClient.sendGetForArray("transactions", params, Transaction::asTransactions,
                        new CompletionHandler<List<Transaction>>() {
                            @Override
                            public void handleData(List<Transaction> transactions) {
                                allTransactions.addAll(transactions);
                                sema.release();
                            }

                            @Override
                            public void handleError(QueryError e) {
                                error[0] = e;
                                sema.release();
                            }
                        });

                sema.acquireUninterruptibly();
            }
        }

        if (error[0] != null) {
            handler.handleError(error[0]);
        } else {
            handler.handleData(allTransactions);
        }
    }
}
