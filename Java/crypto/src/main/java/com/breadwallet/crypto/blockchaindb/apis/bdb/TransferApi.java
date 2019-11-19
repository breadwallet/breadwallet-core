/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.bdb;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.apis.PagedCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transfer;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Lists;
import com.google.common.primitives.UnsignedLong;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;

import static com.google.common.base.Preconditions.checkArgument;

public class TransferApi {

    private static final int ADDRESS_COUNT = 50;

    private final BdbApiClient jsonClient;
    private final ExecutorService executorService;

    public TransferApi(BdbApiClient jsonClient,
                       ExecutorService executorService) {
        this.jsonClient = jsonClient;
        this.executorService = executorService;
    }

    public void getTransfers(String id,
                             List<String> addresses,
                             UnsignedLong beginBlockNumber,
                             UnsignedLong endBlockNumber,
                             @Nullable Integer maxPageSize,
                             CompletionHandler<List<Transfer>, QueryError> handler) {
        List<List<String>> chunkedAddressesList = Lists.partition(addresses, ADDRESS_COUNT);
        GetChunkedCoordinator<String, Transfer> coordinator = new GetChunkedCoordinator<>(chunkedAddressesList, handler);

        for (int i = 0; i < chunkedAddressesList.size(); i++) {
            List<String> chunkedAddresses = chunkedAddressesList.get(i);

            ImmutableListMultimap.Builder<String, String> paramsBuilder = ImmutableListMultimap.builder();
            paramsBuilder.put("blockchain_id", id);
            paramsBuilder.put("start_height", beginBlockNumber.toString());
            paramsBuilder.put("end_height", endBlockNumber.toString());
            if (null != maxPageSize) paramsBuilder.put("max_page_size", maxPageSize.toString());
            for (String address : chunkedAddresses) paramsBuilder.put("address", address);
            ImmutableMultimap<String, String> params = paramsBuilder.build();

            PagedCompletionHandler<List<Transfer>, QueryError> pagedHandler = createPagedResultsHandler(coordinator, chunkedAddresses);
            jsonClient.sendGetForArrayWithPaging("transfers", params, Transfer.class, pagedHandler);
        }
    }

    public void getTransfer(String id,
                            CompletionHandler<Transfer, QueryError> handler) {
        jsonClient.sendGetWithId("transfers", id, ImmutableMultimap.of(), Transfer.class, handler);
    }

    private void submitGetNextTransfers(String nextUrl,
                                        PagedCompletionHandler<List<Transfer>, QueryError> handler) {
        executorService.submit(() -> getNextTransfers(nextUrl, handler));
    }

    private void getNextTransfers(String nextUrl,
                                  PagedCompletionHandler<List<Transfer>, QueryError> handler) {
        jsonClient.sendGetForArrayWithPaging("transfers", nextUrl, Transfer.class, handler);
    }

    private PagedCompletionHandler<List<Transfer>, QueryError> createPagedResultsHandler(GetChunkedCoordinator<String, Transfer> coordinator,
                                                                                         List<String> chunkedAddresses) {
        List<Transfer> allResults = new ArrayList<>();
        return new PagedCompletionHandler<List<Transfer>, QueryError>() {
            @Override
            public void handleData(List<Transfer> results, String prevUrl, String nextUrl) {
                allResults.addAll(results);

                if (nextUrl != null) {
                    submitGetNextTransfers(nextUrl, this);

                } else {
                    coordinator.handleChunkData(chunkedAddresses, allResults);
                }
            }

            @Override
            public void handleError(QueryError error) {
                coordinator.handleError(error);
            }
        };
    }
}
