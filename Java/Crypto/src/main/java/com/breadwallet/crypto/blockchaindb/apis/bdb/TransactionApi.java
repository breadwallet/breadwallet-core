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

import static com.google.common.base.Preconditions.checkState;

public class TransactionApi {

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
        List<List<String>> chunkedAddressesList = Lists.partition(addresses, ADDRESS_COUNT);
        GetTransactionsCoordinator coordinator = new GetTransactionsCoordinator(chunkedAddressesList, handler);

        for (int i = 0; i < chunkedAddressesList.size(); i++) {
            List<String> chunkedAddresses = chunkedAddressesList.get(i);

            ImmutableListMultimap.Builder<String, String> paramsBuilder = ImmutableListMultimap.builder();
            paramsBuilder.put("blockchain_id", id);
            paramsBuilder.put("include_proof", String.valueOf(includeProof));
            paramsBuilder.put("include_raw", String.valueOf(includeRaw));
            paramsBuilder.put("start_height", beginBlockNumber.toString());
            paramsBuilder.put("end_height", endBlockNumber.toString());
            for (String address : chunkedAddresses) paramsBuilder.put("address", address);
            ImmutableMultimap<String, String> params = paramsBuilder.build();

            PagedCompletionHandler<List<Transaction>, QueryError> pagedHandler = coordinator.createPagedResultsHandler(chunkedAddresses);
            jsonClient.sendGetForArrayWithPaging("transactions", params, Transaction::asTransactions, pagedHandler);
        }
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

    private void submitGetNextTransactions(String nextUrl, PagedCompletionHandler<List<Transaction>, QueryError> handler) {
        executorService.submit(() -> getNextTransactions(nextUrl, handler));
    }

    private void getNextTransactions(String nextUrl, PagedCompletionHandler<List<Transaction>, QueryError> handler) {
        jsonClient.sendGetForArrayWithPaging("transactions", nextUrl, Transaction::asTransactions,
                handler);
    }

    private class GetTransactionsCoordinator {

        private final List<List<String>> chunks;
        private final List<Transaction> transactions;
        private final CompletionHandler<List<Transaction>, QueryError> handler;

        private QueryError error;

        private GetTransactionsCoordinator(List<List<String>> chunks, CompletionHandler<List<Transaction>, QueryError> handler) {
            this.chunks = new ArrayList<>(chunks);
            this.transactions = new ArrayList<>();
            this.handler = handler;
        }

        private PagedCompletionHandler<List<Transaction>, QueryError> createPagedResultsHandler(List<String> chunk) {
            List<Transaction> allResults = new ArrayList<>();
            return new PagedCompletionHandler<List<Transaction>, QueryError>() {
                @Override
                public void handleData(List<Transaction> results, PageInfo info) {
                    allResults.addAll(results);

                    if (info.nextUrl == null) {
                        GetTransactionsCoordinator.this.handleChunkData(chunk, allResults);
                    } else {
                        submitGetNextTransactions(info.nextUrl, this);
                    }
                }

                @Override
                public void handleError(QueryError error) {
                    GetTransactionsCoordinator.this.handleError(error);
                }
            };
        }

        private void handleChunkData(List<String> chunk, List<Transaction> data) {
            boolean transitionToSuccess = false;

            synchronized (this) {
                checkState(!isInSuccessState());

                if (!isInErrorState()) {
                    chunks.remove(chunk);
                    transactions.addAll(data);
                    transitionToSuccess = isInSuccessState();
                }
            }

            if (transitionToSuccess) {
                handleSuccess();
            }
        }

        private void handleError(QueryError error) {
            boolean transitionToError = false;

            synchronized (this) {
                checkState(!isInSuccessState());

                if (!isInErrorState()) {
                    this.error = error;
                    transitionToError = isInErrorState();
                }
            }

            if (transitionToError) {
                handleFailure(error);
            }
        }

        private boolean isInErrorState() {
            return error != null;
        }

        private boolean isInSuccessState() {
            return chunks.isEmpty();
        }

        private void handleSuccess() {
            handler.handleData(transactions);
        }

        private void handleFailure(QueryError error) {
            handler.handleError(error);
        }
    }
}
