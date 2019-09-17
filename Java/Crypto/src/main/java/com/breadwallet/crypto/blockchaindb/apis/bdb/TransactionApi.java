/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.apis.PageInfo;
import com.breadwallet.crypto.blockchaindb.apis.PagedCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Lists;
import com.google.common.collect.Multimap;
import com.google.common.io.BaseEncoding;
import com.google.common.primitives.UnsignedLong;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Semaphore;

public class TransactionApi {

    private static final UnsignedLong PAGINATION_COUNT = UnsignedLong.valueOf(5000);
    private static final int ADDRESS_COUNT = 50;

    private final BdbApiClient jsonClient;
    private final ExecutorService executorService;

    public TransactionApi(BdbApiClient jsonClient, ExecutorService executorService) {
        this.jsonClient = jsonClient;
        this.executorService = executorService;
    }

    public void getTransactions(String id, List<String> addresses, UnsignedLong beginBlockNumber, UnsignedLong endBlockNumber,
                                boolean includeRaw, boolean includeProof,
                                CompletionHandler<List<Transaction>, QueryError> handler) {
        executorService.submit(() -> getTransactionsOnExecutor(id, addresses, beginBlockNumber, endBlockNumber,
                includeRaw, includeProof, handler));
    }

    public void getTransaction(String id, boolean includeRaw, boolean includeProof,
                               CompletionHandler<Transaction, QueryError> handler) {
        Multimap<String, String> params = ImmutableListMultimap.of("include_proof", String.valueOf(includeProof),
                "include_raw", String.valueOf(includeRaw));

        jsonClient.sendGetWithId("transactions", id, params, Transaction::asTransaction, handler);
    }

    public void createTransaction(String id, String hashAsHex, byte[] tx, CompletionHandler<Void, QueryError> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "blockchain_id", id,
                "transaction_id", hashAsHex,
                "data", BaseEncoding.base64().encode(tx)));
        jsonClient.sendPost("transactions", ImmutableMultimap.of(), json, handler);
    }

    private void getTransactionsOnExecutor(String id, List<String> addresses, UnsignedLong beginBlockNumber,
                                           UnsignedLong endBlockNumber, boolean includeRaw, boolean includeProof,
                                           CompletionHandler<List<Transaction>, QueryError> handler) {
        final QueryError[] error = {null};
        List<Transaction> allTransactions = new ArrayList<>();
        Semaphore sema = new Semaphore(0);

        List<List<String>> chunkedAddressesList = Lists.partition(addresses, ADDRESS_COUNT);
        for (int i = 0; i < chunkedAddressesList.size() && error[0] == null; i++) {
            List<String> chunkedAddresses = chunkedAddressesList.get(i);

            ImmutableListMultimap.Builder<String, String> paramsBuilder = ImmutableListMultimap.builder();
            paramsBuilder.put("blockchain_id", id);
            paramsBuilder.put("include_proof", String.valueOf(includeProof));
            paramsBuilder.put("include_raw", String.valueOf(includeRaw));
            paramsBuilder.put("start_height", beginBlockNumber.toString());
            paramsBuilder.put("end_height", endBlockNumber.toString());
            for (String address : chunkedAddresses) paramsBuilder.put("address", address);
            ImmutableMultimap<String, String> params = paramsBuilder.build();

            final String[] nextUrl = {null};

            jsonClient.sendGetForArrayWithPaging("transactions", params, Transaction::asTransactions,
                    new PagedCompletionHandler<List<Transaction>, QueryError>() {
                        @Override
                        public void handleData(List<Transaction> transactions, PageInfo info) {
                            nextUrl[0] = info.nextUrl;
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

            while (nextUrl[0] != null && error[0] == null) {
                jsonClient.sendGetForArrayWithPaging("transactions", nextUrl[0], Transaction::asTransactions,
                        new PagedCompletionHandler<List<Transaction>, QueryError>() {
                            @Override
                            public void handleData(List<Transaction> transactions, PageInfo info) {
                                nextUrl[0] = info.nextUrl;
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
