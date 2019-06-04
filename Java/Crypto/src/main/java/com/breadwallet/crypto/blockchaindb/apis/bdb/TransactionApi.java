package com.breadwallet.crypto.blockchaindb.apis.bdb;

import android.util.Base64;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transaction;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Semaphore;

import static com.google.common.base.Preconditions.checkArgument;

public class TransactionApi {

    private static final int PAGINATION_COUNT = 5000;

    private final BdbApiClient jsonClient;
    private final ExecutorService executorService;

    public TransactionApi(BdbApiClient jsonClient, ExecutorService executorService) {
        this.jsonClient = jsonClient;
        this.executorService = executorService;
    }


    // TODO(fix): Are we going to have a problem here with the number of addresses getting to be too big?
    public void getTransactions(String id, List<String> addresses, long beginBlockNumber, long endBlockNumber,
                                boolean includeRaw, boolean includeProof,
                                BlockchainCompletionHandler<List<Transaction>> handler) {
        executorService.submit(() -> getTransactionsOnExecutor(id, addresses, beginBlockNumber, endBlockNumber,
                includeRaw, includeProof, handler));
    }

    public void getTransaction(String id, boolean includeRaw, boolean includeProof,
                               BlockchainCompletionHandler<Transaction> handler) {
        Multimap<String, String> params = ImmutableListMultimap.of("include_proof", String.valueOf(includeProof),
                "include_raw", String.valueOf(includeRaw));

        jsonClient.sendGetWithId("transactions", id, params, Transaction::asTransaction, handler);
    }

    public void putTransaction(String id, byte[] data, BlockchainCompletionHandler<Transaction> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of("transaction", Base64.encode(data, Base64.DEFAULT)));
        Multimap<String, String> params = ImmutableListMultimap.of("blockchain_id", id);
        jsonClient.sendPut("transactions", params, json, Transaction::asTransaction, handler);
    }

    private void getTransactionsOnExecutor(String id, List<String> addresses, long beginBlockNumber,
                                           long endBlockNumber, boolean includeRaw, boolean includeProof,
                                           BlockchainCompletionHandler<List<Transaction>> handler) {
        final QueryError[] error = {null};
        List<Transaction> allTransactions = new ArrayList<>();
        Semaphore sema = new Semaphore(0);

        ImmutableListMultimap.Builder<String, String> baseBuilder = ImmutableListMultimap.builder();
        baseBuilder.put("blockchain_id", id);
        baseBuilder.put("include_proof", String.valueOf(includeProof));
        baseBuilder.put("include_raw", String.valueOf(includeRaw));
        for (String address : addresses) baseBuilder.put("address", address);
        ImmutableMultimap<String, String> baseParams = baseBuilder.build();

        for (long i = beginBlockNumber; i < endBlockNumber && error[0] == null; i += PAGINATION_COUNT) {
            ImmutableListMultimap.Builder<String, String> paramsBuilder = ImmutableListMultimap.builder();
            paramsBuilder.putAll(baseParams);
            paramsBuilder.putAll("start_height", String.valueOf(i));
            paramsBuilder.putAll("end_height", String.valueOf(Math.min(i + PAGINATION_COUNT, endBlockNumber)));
            ImmutableMultimap<String, String> params = paramsBuilder.build();

            jsonClient.sendGetForArray("transactions", params, Transaction::asTransactions,
                    new BlockchainCompletionHandler<List<Transaction>>() {
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

        if (error[0] != null) {
            handler.handleError(error[0]);
        } else {
            handler.handleData(allTransactions);
        }
    }
}
