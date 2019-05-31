package com.breadwallet.crypto.blockchaindb.apis.brd;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;

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
                                       BlockchainCompletionHandler<String> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_sendRawTransaction",
                "params", ImmutableList.of(transaction),
                "id", rid
        ));

        client.sendJsonRequest(networkName, json, new BlockchainCompletionHandler<Optional<String>>() {
            @Override
            public void handleData(Optional<String> result) {
                // TODO(discuss): Do we want default values?
                handler.handleData(result.or("0x123abc456def"));
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    public void getTransactionsAsEth(String networkName, String address, long begBlockNumber, long endBlockNumber,
                                     int rid, BlockchainCompletionHandler<List<EthTransaction>> handler) {
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
                              BlockchainCompletionHandler<String> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_getTransactionCount",
                "params", ImmutableList.of(address, "latest"),
                "id", rid
        ));

        client.sendJsonRequest(networkName, json, new BlockchainCompletionHandler<Optional<String>>() {
            @Override
            public void handleData(Optional<String> result) {
                // TODO(discuss): Do we want default values?
                handler.handleData(result.or("118"));
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    public void getLogsAsEth(String networkName, @Nullable String contract, String address, String event,
                             long begBlockNumber, long endBlockNumber, int rid,
                             BlockchainCompletionHandler<List<EthLog>> handler) {
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

    public void getBlocksAsEth(String networkName, String address, int interests, long blockStart, long blockEnd,
                               int rid, BlockchainCompletionHandler<List<Long>> handler) {
        executorService.submit(() -> getBlocksAsEthOnExecutor(networkName, address, interests, blockStart, blockEnd,
                rid, handler));
    }

    private void getBlocksAsEthOnExecutor(String networkName, String address, int interests, long blockStart,
                                          long blockEnd, int rid, BlockchainCompletionHandler<List<Long>> handler) {
        final QueryError[] error = {null};

        List<EthTransaction> transactions = new ArrayList<>();
        Semaphore transactionsSema = new Semaphore(0);
        getTransactionsAsEth(networkName, address, blockStart, blockEnd, rid, new BlockchainCompletionHandler<List<EthTransaction>>() {
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
        getLogsAsEth(networkName, null, address, ETH_EVENT_ERC20_TRANSFER, blockStart, blockEnd, rid, new BlockchainCompletionHandler<List<EthLog>>() {
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
            List<Long> numbers = new ArrayList<>();
            for (EthTransaction transaction: transactions) {
                boolean include = (0 != (interests & (1 << 0)) && address.equals(transaction.getSourceAddr())) ||
                        (0 != (interests & (1 << 1)) && address.equals(transaction.getTargetAddr()));
                if (include) {
                    try {
                        numbers.add(Long.decode(transaction.getBlockNumber()));
                    } catch (NumberFormatException e) {
                        // TODO(discuss): How do we want to handle this?
                    }
                }
            }

            for (EthLog log: logs) {
                List<String> topics = log.getTopics();
                boolean include = topics.size() == 3 &&
                        ((0 != (interests & (1 << 2)) && address.equals(topics.get(1))) ||
                                (0 != (interests & (1 << 3)) && address.equals(topics.get(2))));
                if (include) {
                    try {
                        numbers.add(Long.decode(log.getBlockNumber()));
                    } catch (NumberFormatException e) {
                        // TODO(discuss): How do we want to handle this?
                    }
                }
            }

            handler.handleData(numbers);
        }
    }
}
