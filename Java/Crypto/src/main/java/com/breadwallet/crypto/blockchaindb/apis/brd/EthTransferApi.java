/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.brd;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.CompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.google.common.primitives.UnsignedLongs;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Semaphore;

public class EthTransferApi {

    private static final String ETH_EVENT_ERC20_TRANSFER = "0xa9059cbb";

    private final BrdApiClient client;
    private final ExecutorService executorService;

    public EthTransferApi(BrdApiClient client, ExecutorService executorService) {
        this.client = client;
        this.executorService = executorService;
    }

    public void submitTransactionAsEth(String networkName, String transaction, int rid,
                                       CompletionHandler<String> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_sendRawTransaction",
                "params", ImmutableList.of(transaction),
                "id", rid
        ));

        client.sendJsonRequest(networkName, json, handler);
    }

    public void getTransactionsAsEth(String networkName, String address, UnsignedLong begBlockNumber, UnsignedLong endBlockNumber,
                                     int rid, CompletionHandler<List<EthTransaction>> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "id", rid,
                "account", address
        ));

        ImmutableMultimap<String, String> params = ImmutableListMultimap.of(
                "module", "account",
                "action", "txlist",
                "address", address,
                "startBlock", String.valueOf(begBlockNumber),
                "endBlock", String.valueOf(endBlockNumber)
        );

        client.sendQueryForArrayRequest(networkName, params, json, EthTransaction::asTransactions, handler);
    }

    public void getNonceAsEth(String networkName, String address, int rid,
                              CompletionHandler<String> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_getTransactionCount",
                "params", ImmutableList.of(address, "latest"),
                "id", rid
        ));

        client.sendJsonRequest(networkName, json, handler);
    }

    public void getLogsAsEth(String networkName, @Nullable String contract, String address, String event,
                             UnsignedLong begBlockNumber, UnsignedLong endBlockNumber, int rid,
                             CompletionHandler<List<EthLog>> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "id", rid
        ));

        ImmutableListMultimap.Builder<String, String> paramsBuilders = ImmutableListMultimap.builder();
        paramsBuilders.put("module", "logs");
        paramsBuilders.put("action", "getLogs");
        paramsBuilders.put("fromBlock", String.valueOf(begBlockNumber));
        paramsBuilders.put("toBlock", String.valueOf(endBlockNumber));
        paramsBuilders.put("topic0", event);
        paramsBuilders.put("topic1", address);
        paramsBuilders.put("topic_1_2_opr", "or");
        paramsBuilders.put("topic2", address);

        if (null != contract) {
            paramsBuilders.put("address", contract);
        }

        client.sendQueryForArrayRequest(networkName, paramsBuilders.build(), json, EthLog::asLogs, handler);
    }

    public void getBlocksAsEth(String networkName, String address, UnsignedInteger interests, UnsignedLong blockStart, UnsignedLong blockEnd,
                               int rid, CompletionHandler<List<UnsignedLong>> handler) {
        executorService.submit(() -> getBlocksAsEthOnExecutor(networkName, address, interests, blockStart, blockEnd,
                rid, handler));
    }

    private void getBlocksAsEthOnExecutor(String networkName, String address, UnsignedInteger interests, UnsignedLong blockStart,
                                          UnsignedLong blockEnd, int rid, CompletionHandler<List<UnsignedLong>> handler) {
        final QueryError[] error = {null};

        List<EthTransaction> transactions = new ArrayList<>();
        Semaphore transactionsSema = new Semaphore(0);
        getTransactionsAsEth(networkName, address, blockStart, blockEnd, rid, new CompletionHandler<List<EthTransaction>>() {
            @Override
            public void handleData(List<EthTransaction> data) {
                transactions.addAll(data);
                transactionsSema.release();
            }

            @Override
            public void handleError(QueryError e) {
                error[0] = e;
                transactionsSema.release();
            }
        });

        List<EthLog> logs = new ArrayList<>();
        Semaphore logsSema = new Semaphore(0);
        getLogsAsEth(networkName, null, address, ETH_EVENT_ERC20_TRANSFER, blockStart, blockEnd, rid, new CompletionHandler<List<EthLog>>() {
            @Override
            public void handleData(List<EthLog> data) {
                logs.addAll(data);
                logsSema.release();
            }

            @Override
            public void handleError(QueryError e) {
                error[0] = e;
                logsSema.release();
            }
        });

        transactionsSema.acquireUninterruptibly();
        logsSema.acquireUninterruptibly();

        if (error[0] != null) {
            handler.handleError(error[0]);

        } else {
            List<UnsignedLong> numbers = new ArrayList<>();
            int interestsAsInt = interests.intValue();

            for (EthTransaction transaction: transactions) {

                boolean include = (0 != (interestsAsInt & (1 << 0)) && address.equals(transaction.getSourceAddr())) ||
                        (0 != (interestsAsInt & (1 << 1)) && address.equals(transaction.getTargetAddr()));
                if (include) {
                    try {
                        numbers.add(UnsignedLong.fromLongBits(UnsignedLongs.decode(transaction.getBlockNumber())));
                    } catch (NumberFormatException e) {
                        handler.handleError(new QueryModelError("Invalid transaction block number"));
                        return;
                    }
                }
            }

            for (EthLog log: logs) {
                List<String> topics = log.getTopics();
                boolean include = topics.size() == 3 &&
                        ((0 != (interestsAsInt & (1 << 2)) && address.equals(topics.get(1))) ||
                                (0 != (interestsAsInt & (1 << 3)) && address.equals(topics.get(2))));
                if (include) {
                    try {
                        numbers.add(UnsignedLong.fromLongBits(UnsignedLongs.decode(log.getBlockNumber())));
                    } catch (NumberFormatException e) {
                        handler.handleError(new QueryModelError("Invalid log block number"));
                        return;
                    }
                }
            }

            handler.handleData(numbers);
        }
    }
}
