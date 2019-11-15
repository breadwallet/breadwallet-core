/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.brd;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;
import com.google.common.primitives.UnsignedLongs;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import static com.google.common.base.Preconditions.checkState;

public class EthTransferApi {

    private static final String ETH_EVENT_ERC20_TRANSFER = "0xa9059cbb";

    private final BrdApiClient client;

    public EthTransferApi(BrdApiClient client) {
        this.client = client;
    }

    public void submitTransactionAsEth(String networkName,
                                       String transaction,
                                       int rid,
                                       CompletionHandler<String, QueryError> handler) {
        Map json = ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_sendRawTransaction",
                "params", ImmutableList.of(transaction),
                "id", rid
        );

        client.sendJsonRequest(networkName, json, handler);
    }

    public void getTransactionsAsEth(String networkName,
                                     String address,
                                     UnsignedLong begBlockNumber,
                                     UnsignedLong endBlockNumber,
                                     int rid,
                                     CompletionHandler<List<EthTransaction>, QueryError> handler) {
        Map json = ImmutableMap.of(
                "id", rid,
                "account", address
        );

        ImmutableMultimap<String, String> params = ImmutableListMultimap.of(
                "module", "account",
                "action", "txlist",
                "address", address,
                "startBlock", String.valueOf(begBlockNumber),
                "endBlock", String.valueOf(endBlockNumber)
        );

        client.sendQueryForArrayRequest(networkName, params, json, EthTransaction.class, handler);
    }

    public void getNonceAsEth(String networkName,
                              String address,
                              int rid,
                              CompletionHandler<String, QueryError> handler) {
        Map json = ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_getTransactionCount",
                "params", ImmutableList.of(address, "latest"),
                "id", rid
        );

        client.sendJsonRequest(networkName, json, handler);
    }

    public void getLogsAsEth(String networkName,
                             @Nullable String contract,
                             String address,
                             String event,
                             UnsignedLong begBlockNumber,
                             UnsignedLong endBlockNumber,
                             int rid,
                             CompletionHandler<List<EthLog>, QueryError> handler) {
        Map json = ImmutableMap.of(
                "id", rid
        );

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

        client.sendQueryForArrayRequest(networkName, paramsBuilders.build(), json, EthLog.class, handler);
    }

    public void getBlocksAsEth(String networkName,
                               String address,
                               UnsignedInteger interests,
                               UnsignedLong blockStart,
                               UnsignedLong blockEnd,
                               int rid,
                               CompletionHandler<List<UnsignedLong>, QueryError> handler) {
        GetBlocksCoordinator coordinator = new GetBlocksCoordinator(address, interests, handler);

        getTransactionsAsEth(networkName, address, blockStart, blockEnd, rid, coordinator.createTxnHandler());
        getLogsAsEth(networkName, null, address, ETH_EVENT_ERC20_TRANSFER, blockStart, blockEnd, rid, coordinator.createLogHandler());
    }

    private static class GetBlocksCoordinator {

        private final String address;
        private final UnsignedInteger interests;
        private final CompletionHandler<List<UnsignedLong>, QueryError> handler;

        private List<EthTransaction> transactions;
        private List<EthLog> logs;
        private QueryError error;

        private GetBlocksCoordinator(String address, UnsignedInteger interests, CompletionHandler<List<UnsignedLong>, QueryError> handler) {
            this.address = address;
            this.interests = interests;
            this.handler = handler;
        }

        private CompletionHandler<List<EthTransaction>, QueryError> createTxnHandler() {
            return new CompletionHandler<List<EthTransaction>, QueryError>() {
                @Override
                public void handleData(List<EthTransaction> data) {
                    GetBlocksCoordinator.this.handleTxnData(data);
                }

                @Override
                public void handleError(QueryError e) {
                    GetBlocksCoordinator.this.handleError(e);
                }
            };
        }

        private CompletionHandler<List<EthLog>, QueryError> createLogHandler() {
            return new CompletionHandler<List<EthLog>, QueryError>() {
                @Override
                public void handleData(List<EthLog> data) {
                    GetBlocksCoordinator.this.handleLogData(data);
                }

                @Override
                public void handleError(QueryError e) {
                    GetBlocksCoordinator.this.handleError(e);
                }
            };
        }

        private void handleTxnData(List<EthTransaction> txns) {
            boolean transitionToSuccess = false;

            synchronized (this) {
                checkState(!isInSuccessState());

                if (!isInErrorState()) {
                    this.transactions = txns;
                    transitionToSuccess = isInSuccessState();
                }
            }

            if (transitionToSuccess) {
                handleSuccess();
            }
        }

        private void handleLogData(List<EthLog> logs) {
            boolean transitionToSuccess = false;

            synchronized (this) {
                checkState(!isInSuccessState());

                if (!isInErrorState()) {
                    this.logs = logs;
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
            return transactions != null && logs != null;
        }

        private void handleSuccess() {
            List<UnsignedLong> numbers = new ArrayList<>();
            int interestsAsInt = interests.intValue();

            for (EthTransaction transaction : transactions) {

                boolean include = (0 != (interestsAsInt & (1)) && address.equalsIgnoreCase(transaction.getSourceAddr())) ||
                        (0 != (interestsAsInt & (1 << 1)) && address.equalsIgnoreCase(transaction.getTargetAddr()));
                if (include) {
                    try {
                        numbers.add(UnsignedLong.fromLongBits(UnsignedLongs.decode(transaction.getBlockNumber())));
                    } catch (NumberFormatException e) {
                        handler.handleError(new QueryModelError("Invalid transaction block number"));
                        return;
                    }
                }
            }

            for (EthLog log : logs) {
                List<String> topics = log.getTopics();
                boolean include = topics.size() == 3 &&
                        ((0 != (interestsAsInt & (1 << 2)) && address.equalsIgnoreCase(topics.get(1))) ||
                                (0 != (interestsAsInt & (1 << 3)) && address.equalsIgnoreCase(topics.get(2))));
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

        private void handleFailure(QueryError error) {
            handler.handleError(error);
        }
    }
}
